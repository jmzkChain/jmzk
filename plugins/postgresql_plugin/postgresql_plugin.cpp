/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/postgresql_plugin/postgresql_plugin.hpp>
#include <evt/postgresql_plugin/evt_interpreter.hpp>
#include <evt/postgresql_plugin/write_context.hpp>

extern "C" {
#include <evt/postgresql_plugin/evt_db.h>
}

#include <functional>
#include <queue>
#include <tuple>
#include <thread>

#if __has_include(<condition>)
#include <condition>
using std::condition_variable_any;
#else
#include <boost/thread/condition.hpp>
using boost::condition_variable_any;
#endif

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>

#include <evt/chain/config.hpp>
#include <evt/chain/genesis_state.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/utilities/spinlock.hpp>

namespace fc {
class variant;
}

namespace evt {

using namespace chain;
using namespace chain::contracts;
using namespace chain::plugin_interface;

using evt::utilities::spinlock;
using evt::utilities::spinlock_guard;

static appbase::abstract_plugin& _postgresql_plugin = app().register_plugin<postgresql_plugin>();

class postgresql_plugin_impl {
private:
    using inblock_ptr = std::tuple<block_state_ptr, bool>; // true for irreversible block

public:
    postgresql_plugin_impl()
        : evt_abi(evt_contract_abi(), fc::hours(1))
        , db_conn_(nullptr)
    { }

    ~postgresql_plugin_impl();

public:
    void consume_queues();

    void applied_block(const block_state_ptr&);
    void applied_irreversible_block(const block_state_ptr&);
    void applied_transaction(const transaction_trace_ptr&);

    void process_block(const signed_block&, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx);
    void _process_block(const signed_block&, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx);
    void process_irreversible_block(const signed_block&, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx);
    void _process_irreversible_block(const signed_block&, write_context& write_ctx);
    void process_transaction(const transaction_trace&, write_context& write_ctx);
    void _process_transaction(const transaction_trace&, write_context& write_ctx);

    void add_trx_trace(bsoncxx::builder::basic::document& trx_doc,
                       const chain::transaction& trx,
                       std::deque<transaction_trace_ptr>& traces,
                       std::function<void(action&)>&& on_paycharge_act);

    void add_trx_ext(bsoncxx::builder::basic::document& trx_doc, const chain::transaction& trx);

    void init();
    void wipe_database();

public:
    abi_serializer              evt_abi;
    fc::optional<chain_id_type> chain_id;

    bool configured{false};
    bool wipe_database_on_startup{false};

    void* db_conn_;

    evt_interpreter    interpreter;

    size_t processed  = 0;
    size_t queue_size = 0;

    std::deque<inblock_ptr>           block_state_queue;
    std::deque<transaction_trace_ptr> transaction_trace_queue;

    spinlock                    lock_;
    condition_variable_any      cond_;
    std::thread                 consume_thread_;
    std::atomic_bool            done_{false};

    write_context write_ctx_;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
    fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
    fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

public:
    const std::string blocks_col        = "Blocks";
    const std::string trxs_col          = "Transactions";
    const std::string actions_col       = "Actions";
    const std::string domains_col       = "Domains";
    const std::string tokens_col        = "Tokens";
    const std::string groups_col        = "Groups";
    const std::string fungibles_col     = "Fungibles";
};



template <typename Q, typename V>
inline void
queueb(Q& bqueue, V&& v, spinlock& lock, condition_variable_any& cv, size_t queue_size) {
    lock.lock();

    auto sleep_time = 0ul;

    while(bqueue.size() > queue_size) {
        lock.unlock();
        cv.notify_one();

        sleep_time += 100;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

        lock.lock();
    }
    bqueue.emplace_back(std::forward<V>(v));
    lock.unlock();
    cv.notify_one();
}

template <typename Q, typename V>
inline void
queuet(Q& tqueue, V&& v, spinlock& lock, condition_variable_any& cv) {
    lock.lock();
    tqueue.emplace_back(std::forward<V>(v));
    lock.unlock();
    cv.notify_one();
}

void
postgresql_plugin_impl::applied_irreversible_block(const block_state_ptr& bsp) {
    queueb(block_state_queue, std::make_tuple(bsp, true), lock_, cond_, queue_size);
}

void
postgresql_plugin_impl::applied_block(const block_state_ptr& bsp) {
    queueb(block_state_queue, std::make_tuple(bsp, false), lock_, cond_, queue_size);
}

void
postgresql_plugin_impl::applied_transaction(const transaction_trace_ptr& ttp) {
    queuet(transaction_trace_queue, ttp, lock_, cond_);
}

void
postgresql_plugin_impl::consume_queues() {
    using namespace evt::__internal;

    try {
        while(true) {
            lock_.lock();
            while(block_state_queue.empty() && !done_) {
                cond_.wait(lock_);
            }

            auto bqueue = std::move(block_state_queue);
            auto traces = std::move(transaction_trace_queue);

            lock_.unlock();

            const int BlockPtr       = 0;
            const int IsIrreversible = 1;

            // warn if queue size greater than 75%
            if(bqueue.size() > (queue_size * 0.75)) {
                wlog("queue size: ${q}, head block num: ${b}", ("q", bqueue.size())("b",std::get<BlockPtr>(bqueue.front())->block_num));
            }
            else if(done_) {
                ilog("draining queue, size: ${q}", ("q", bqueue.size()));
                break;
            }

            // process block states
            while(true) {
                if(bqueue.empty()) {
                    break;
                }

                auto& b = bqueue.front();
                if(std::get<IsIrreversible>(b)) {
                    process_irreversible_block(*(std::get<BlockPtr>(b)->block), traces, write_ctx_);
                }
                else {
                    process_block(*(std::get<BlockPtr>(b)->block), traces, write_ctx_);
                }

                if(write_ctx_.total() >= queue_size * 2) {
                    write_ctx_.execute();
                }

                bqueue.pop_front();
            }
            if(write_ctx_.total() > 0) {
                write_ctx_.execute();
            }

            if(!traces.empty()) {
                spinlock_guard lock(lock_);
                transaction_trace_queue.insert(transaction_trace_queue.begin(), traces.begin(), traces.end());
            }
        }
        ilog("postgresql_plugin consume thread shutdown gracefully");
    }
    catch(fc::exception& e) {
        elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while consuming block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while consuming block");
    }
}

namespace __internal {

void
add_data(bsoncxx::builder::basic::document& act_doc,
         const chain::action&               act,
         const abi_serializer&              evt_abi) {
    using bsoncxx::builder::basic::kvp;
    try {
        auto& abis = evt_abi;
        auto v     = abis.binary_to_variant(abis.get_action_type(act.name), act.data);
        auto json  = fc::json::to_string(v);
        try {
            const auto& value = bsoncxx::from_json(json);
            act_doc.append(kvp("data", value));
            return;
        }
        catch(std::exception& e) {
            elog("Unable to convert EVT JSON to MongoDB JSON: ${e}", ("e", e.what()));
            elog("  EVT JSON: ${j}", ("j", json));
        }
    }
    catch(fc::exception& e) {
        elog("Unable to convert action.data to ABI: ${n}, what: ${e}", ("n", act.name)("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("Unable to convert action.data to ABI: ${n}, std what: ${e}", ("n", act.name)("e", e.what()));
    }
    catch(...) {
        elog("Unable to convert action.data to ABI: ${n}, unknown exception", ("n", act.name));
    }
    // if anything went wrong just store raw hex_data
    act_doc.append(kvp("hex_data", fc::variant(act.data).as_string()));
}

void
verify_last_block(mongocxx::collection& blocks, const std::string& prev_block_id) {
    mongocxx::options::find opts;
    opts.sort(bsoncxx::from_json(R"xxx({ "block_num" : -1 })xxx"));
    auto last_block = blocks.find_one({}, opts);
    if(!last_block) {
        FC_THROW("No blocks found in database");
    }
    const auto id = last_block->view()["block_id"].get_utf8().value.to_string();
    if(id != prev_block_id) {
        FC_THROW("Did not find expected block ${pid}, instead found ${id}", ("pid", prev_block_id)("id", id));
    }
}

void
verify_no_blocks(mongocxx::collection& blocks) {
    if(blocks.count(bsoncxx::from_json("{}")) > 0) {
        FC_THROW("Existing blocks found in database");
    }
}

}  // namespace __internal

void
postgresql_plugin_impl::process_irreversible_block(const signed_block& block, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx) {
    try {
        if(block.block_num() == 1) {
            // genesis block will not trigger on_block event
            // add it manually
            _process_block(block, traces, write_ctx);
        }
        _process_irreversible_block(block, write_ctx);
    }
    catch(fc::exception& e) {
        elog("FC Exception while processing irreversible block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while processing irreversible block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while processing irreversible block");
    }
}

void
postgresql_plugin_impl::process_block(const signed_block& block, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx) {
    try {
        _process_block(block, traces, write_ctx);

        for(auto& ptrx : block.transactions) {
            auto trx = ptrx.trx.get_transaction();
            interpreter.process_trx(trx, write_ctx);
        }
    }
    catch(fc::exception& e) {
        elog("FC Exception while processing block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while processing block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while processing block");
    }
}

void
postgresql_plugin_impl::process_transaction(const transaction_trace& trace, write_context& write_ctx) {
    return;
}

void
postgresql_plugin_impl::_process_block(const signed_block& block, std::deque<transaction_trace_ptr>& traces, write_context& write_ctx) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto blocks = mongo_db[blocks_col];

    auto       block_doc         = bsoncxx::builder::basic::document{};
    const auto block_id          = block.id();
    const auto block_id_str      = block_id.str();
    const auto prev_block_id_str = block.previous.str();
    auto       block_num         = (int32_t)block.block_num();

    if(processed == 0) {
        if(block_num <= 2) {
            // verify on start we have no previous blocks
            verify_no_blocks(blocks);
        }
        else {
            // verify on restart we have previous block
            verify_last_block(blocks, prev_block_id_str);
        }
    }

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto ts = std::chrono::milliseconds{std::chrono::seconds{block.timestamp.operator fc::time_point().sec_since_epoch()}};

    block_doc.append(kvp("block_num", b_int32{block_num}),
                     kvp("block_id", block_id_str),
                     kvp("prev_block_id", prev_block_id_str),
                     kvp("timestamp", b_date{ts}),
                     kvp("trx_merkle_root", block.transaction_mroot.str()),
                     kvp("trx_count", b_int32{(int)block.transactions.size()}),
                     kvp("producer", block.producer.to_string()),
                     kvp("pending", b_bool{true}),
                     kvp("created_at", b_date{now}));

    write_ctx.get_blocks().append(insert_one(block_doc.view()));

    int32_t act_num        = 0;
    auto    process_action = [&](const std::string& trans_id_str, const chain::action& msg) -> auto {
        auto msg_oid = bsoncxx::oid{};
        auto doc     = bsoncxx::builder::basic::document{};

        doc.append(kvp("_id", b_oid{msg_oid}),
                   kvp("trx_id", trans_id_str),
                   kvp("seq_num", b_int32{act_num}),
                   kvp("block_num", b_int32{block_num}),
                   kvp("name", msg.name.to_string()),
                   kvp("domain", msg.domain.to_string()),
                   kvp("key", msg.key.to_string()),
                   kvp("created_at", b_date{now}));

        add_data(doc, msg, evt_abi);
        write_ctx.get_actions().append(insert_one(doc.view()));
    };

    int32_t trx_num  = 0;
    auto process_trx = [&](const chain::transaction& trx, auto status) -> auto {
        auto       txn_oid      = bsoncxx::oid{};
        auto       doc          = bsoncxx::builder::basic::document{};
        auto       trx_id       = trx.id();
        const auto trans_id_str = trx_id.str();
        
        doc.append(kvp("_id", txn_oid),
                   kvp("trx_id", trans_id_str),
                   kvp("seq_num", b_int32{trx_num}),
                   kvp("block_id", block_id_str),
                   kvp("block_num", b_int32{block_num}),
                   kvp("timestamp", b_date{ts}),
                   kvp("action_count", b_int32{(int)trx.actions.size()}),
                   kvp("expiration",
                       b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}),
                   kvp("max_charge", b_int32{(int32_t)trx.max_charge}),
                   kvp("payer", (std::string)trx.payer),
                   kvp("pending", b_bool{true}),
                   kvp("created_at", b_date{now}));
        // add all input actions in this trx
        if(status == transaction_receipt_header::executed && !trx.actions.empty()) {
            act_num = 0;
            for(const auto& act : trx.actions) {
                process_action(trans_id_str, act);
                act_num++;
            }
        }

        if(!trx.transaction_extensions.empty()) {
            add_trx_ext(doc, trx);
        }

        // add trace(elapsed and charge) and paycharge action
        add_trx_trace(doc, trx, traces, [&](auto& act) {
            process_action(trans_id_str, act);
        });

        return doc;
    };

    trx_num = 0;
    for(const auto& trx_receipt : block.transactions) {
        const auto& trx = trx_receipt.trx.get_signed_transaction();
        auto        doc = process_trx(trx, trx_receipt.status);

        doc.append(kvp("type", (std::string)trx_receipt.type));
        doc.append(kvp("status", (std::string)trx_receipt.status));

        doc.append(kvp("signatures", [&trx](bsoncxx::builder::basic::sub_array subarr) {
            for(const auto& sig : trx.signatures) {
                subarr.append((std::string)sig);
            }
        }));

        auto keys = trx.get_signature_keys(*chain_id);
        doc.append(kvp("keys", [&keys](bsoncxx::builder::basic::sub_array subarr) {
            for(const auto& key : keys) {
                subarr.append((std::string)key);
            }
        }));

        write_ctx.get_trxs().append(insert_one(doc.view()));
        ++trx_num;
    }

    ++processed;
}

void
postgresql_plugin_impl::_process_irreversible_block(const signed_block& block, write_context& write_ctx) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace bsoncxx::builder::stream;
    using namespace mongocxx::model;

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto upd = document();
    upd << "$set" << open_document << "pending" << b_bool{false}
        << "updated_at" << b_date{now}
        << close_document;

    auto filter = document();
    filter << "block_id" << block.id().str();

    write_ctx.get_blocks().append(update_one(filter.view(), upd.view()));
    write_ctx.get_trxs().append(update_many(filter.view(), upd.view()));
}

void
postgresql_plugin_impl::_process_transaction(const transaction_trace& trace, write_context& write_ctx) {
    return;
}

void
postgresql_plugin_impl::add_trx_trace(bsoncxx::builder::basic::document& trx_doc,
                                    const chain::transaction&          trx,
                                    std::deque<transaction_trace_ptr>& traces,
                                    std::function<void(action&)>&&     on_paycharge_act) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto trace_doc = bsoncxx::builder::basic::document{};

    auto it = traces.begin();
    while(it != traces.end()) {
        auto trace = *it;
        traces.pop_front();

        if(trace->id == trx.id()) {
            trace_doc.append(kvp("elapsed", (int64_t)trace->elapsed.count()),
                             kvp("charge", (int64_t)trace->charge));
            trx_doc.append(kvp("trace", trace_doc));
            if(trace->action_traces.empty()) {
                return;
            }
            // because paycharage action is alwasys the latest action
            // only check latest
            auto& act = trace->action_traces.back().act;
            if(act.name == N(paycharge)) {
                on_paycharge_act(act);
            }
            return;
        }
        it++;
    }
}

void
postgresql_plugin_impl::add_trx_ext(bsoncxx::builder::basic::document& trx_doc, const chain::transaction& trx) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using namespace mongocxx::model;
    using bsoncxx::builder::basic::kvp;

    auto ext_doc = bsoncxx::builder::basic::document{};

    for(auto& ext : trx.transaction_extensions) {
        if(std::get<0>(ext) == (uint16_t)chain::transaction_ext::suspend_name) {
            auto& v    = std::get<1>(ext);
            auto  name = string(v.cbegin(), v.cend());

            ext_doc.append(kvp("suspend_name", name));
        }
    }
    trx_doc.append(kvp("exts", ext_doc));
}

postgresql_plugin_impl::~postgresql_plugin_impl() {
    if(!configured) {
        return;
    }
    try {
        done_ = true;
        cond_.notify_one();

        consume_thread_.join();
    }
    catch(std::exception& e) {
        elog("Exception on postgresql_plugin shutdown of consume thread: ${e}", ("e", e.what()));
    }
}

void
postgresql_plugin_impl::wipe_database() {
    ilog("mongo db wipe_database");

    auto blocks        = mongo_db[blocks_col];
    auto trxs          = mongo_db[trxs_col];
    auto actions       = mongo_db[actions_col];
    auto domains       = mongo_db[domains_col];
    auto tokens        = mongo_db[tokens_col];
    auto groups        = mongo_db[groups_col];
    auto fungibles     = mongo_db[fungibles_col];

    blocks.drop();
    trxs.drop();
    actions.drop();
    domains.drop();
    tokens.drop();
    groups.drop();
    fungibles.drop();
}

void
postgresql_plugin_impl::init() {
    using namespace bsoncxx::types;

    auto blocks = mongo_db[blocks_col];  // Blocks
    bsoncxx::builder::stream::document doc{};

    auto need_init = blocks.count(doc.view()) == 0;
    if(need_init) {
        // Blocks indexes
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

        // Transactions indexes
        auto trans = mongo_db[trxs_col];  // Transactions
        trans.create_index(bsoncxx::from_json(R"xxx({ "trx_id" : 1 })xxx"));
        trans.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

        // Action indexes
        auto acts = mongo_db[actions_col];  // Actions
        acts.create_index(bsoncxx::from_json(R"xxx({ "domain" : 1 })xxx"));
        acts.create_index(bsoncxx::from_json(R"xxx({ "trx_id" : 1 })xxx"));

        // Domains indexes
        auto domains = mongo_db[domains_col];
        domains.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

        // Tokens indexes
        auto tokens = mongo_db[tokens_col];
        tokens.create_index(bsoncxx::from_json(R"xxx({ "token_id" : 1 })xxx"));

        // Groups indexes
        auto groups = mongo_db[groups_col];
        groups.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

        // Fungibles indexes
        auto fungibles = mongo_db[fungibles_col];
        fungibles.create_index(bsoncxx::from_json(R"xxx({ "sym_id" : 1 })xxx"));
    }

    write_ctx_.blocks_collection    = mongo_db[blocks_col];
    write_ctx_.trxs_collection      = mongo_db[trxs_col];
    write_ctx_.actions_collection   = mongo_db[actions_col];
    write_ctx_.domains_collection   = mongo_db[domains_col];
    write_ctx_.tokens_collection    = mongo_db[tokens_col];
    write_ctx_.groups_collection    = mongo_db[groups_col];
    write_ctx_.fungibles_collection = mongo_db[fungibles_col];

    // initilize evt interpreter
    interpreter.initialize_db(mongo_db);

    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));

    irreversible_block_connection.emplace(chain.irreversible_block.connect([&](const chain::block_state_ptr& bs) {
        applied_irreversible_block(bs);
    }));

    applied_transaction_connection.emplace(chain.applied_transaction.connect([&](const chain::transaction_trace_ptr& t) {
        applied_transaction(t);
    }));

    if(need_init) {
        // HACK: Add EVT and PEVT manually
        auto gs = chain::genesis_state();

        auto get_nfact = [](auto& f) {
            auto nf = newfungible();

            nf.name         = f.name;
            nf.sym_name     = f.sym_name;
            nf.sym          = f.sym;
            nf.creator      = f.creator;
            nf.issue        = std::move(f.issue);
            nf.manage       = std::move(f.manage);
            nf.total_supply = f.total_supply;

            auto nfact = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
            return nfact;
        };

        auto ng  = newgroup();
        ng.name  = N128(.everiToken);
        ng.group = gs.evt_org;

        // HACK: construct one trx trace here to add EVT and PEVT fungibles to mongodb
        auto trx  = transaction();
        trx.actions.emplace_back(get_nfact(gs.evt));
        trx.actions.emplace_back(get_nfact(gs.pevt));
        trx.actions.emplace_back(action(N128(.group), N128(.everiToken), ng));

        interpreter.process_trx(trx, write_ctx_);
        write_ctx_.execute();
    }
}

////////////
// postgresql_plugin
////////////

postgresql_plugin::postgresql_plugin()
    : my_(new postgresql_plugin_impl) {
}

postgresql_plugin::~postgresql_plugin() {
}

const mongocxx::uri&
postgresql_plugin::uri() const {
    return my_->mongo_uri;
}

bool
postgresql_plugin::enabled() const {
    return my_->configured;
}

void
postgresql_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("mongodb-queue-size,q", bpo::value<uint>()->default_value(5120), "The queue size between evtd and MongoDB plugin thread.")
        ("mongodb-uri,m", bpo::value<std::string>(), "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/."
                                                     " If not specified then plugin is disabled. Default database 'EVT' is used if not specified in URI.")
        ;
}

void
postgresql_plugin::plugin_initialize(const variables_map& options) {
    if(options.count("mongodb-uri")) {
        ilog("initializing postgresql_plugin");
        my_->configured = true;

        if(options.at("replay-blockchain").as<bool>() || options.at("hard-replay-blockchain").as<bool>()) {
            ilog("Replay requested: wiping mongo database on startup");
            my_->wipe_database_on_startup = true;
        } 
        if(options.at("delete-all-blocks").as<bool>()) {
            ilog("Deleted all blocks: wiping mongo database on startup");
            my_->wipe_database_on_startup = true;
        }
        if(options.count("import-reversible-blocks")) {
            ilog("Importing reversible blocks: wiping mongo database on startup");
            my_->wipe_database_on_startup = true;
        }
        
        if(options.count("mongodb-queue-size")) {
            auto size       = options.at("mongodb-queue-size").as<uint>();
            my_->queue_size = size;
        }

        std::string uri_str = options.at("mongodb-uri").as<std::string>();
        ilog("connecting to ${u}", ("u", uri_str));
        
        auto uri    = mongocxx::uri{uri_str};
        auto dbname = uri.database();
        if(dbname.empty()) {
            dbname = "EVT";
        }

        my_->mongo_uri  = std::move(uri);
        my_->mongo_conn = mongocxx::client{my_->mongo_uri};
        my_->mongo_db   = my_->mongo_conn[dbname];
        my_->chain_id   = app().get_plugin<chain_plugin>().chain().get_chain_id();

        if(my_->wipe_database_on_startup) {
            my_->wipe_database();
        }

        my_->init();

        my_->consume_thread_ = std::thread([this] { my_->consume_queues(); });
    }
    else {
        wlog("evt::postgresql_plugin configured, but no --mongodb-uri specified.");
        wlog("postgresql_plugin disabled.");
    }
}

void
postgresql_plugin::plugin_startup() {}

void
postgresql_plugin::plugin_shutdown() {
    my_->accepted_block_connection.reset();
    my_->irreversible_block_connection.reset();
    my_->applied_transaction_connection.reset();
    my_.reset();
}

}  // namespace evt
