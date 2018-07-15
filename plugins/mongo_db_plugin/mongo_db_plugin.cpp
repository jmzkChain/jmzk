/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/mongo_db_plugin/mongo_db_plugin.hpp>
#include <evt/mongo_db_plugin/evt_interpreter.hpp>

#include <queue>
#include <tuple>
#include <mutex>

#include <evt/chain/config.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace fc {
class variant;
}

namespace evt {

using namespace chain;
using namespace chain::contracts;
using namespace chain::plugin_interface;

static appbase::abstract_plugin& _mongo_db_plugin = app().register_plugin<mongo_db_plugin>();

class mongo_db_plugin_impl {
private:
    using inblock_ptr = std::tuple<block_state_ptr, bool>; // true for irreversible block

public:
    mongo_db_plugin_impl()
        : mongo_inst{}
        , mongo_conn{}
    { }

    ~mongo_db_plugin_impl();

public:
    void consume_queues();

    void applied_block(const block_state_ptr&);
    void applied_irreversible_block(const block_state_ptr&);
    void applied_transaction(const transaction_trace_ptr&);
    void process_block(const signed_block&);
    void _process_block(const signed_block&);
    void process_irreversible_block(const signed_block&);
    void _process_irreversible_block(const signed_block&);
    void process_transaction(const transaction_trace&);
    void _process_transaction(const transaction_trace&);

    void init();
    void wipe_database();

public:
    abi_serializer              evt_abi;
    fc::optional<chain_id_type> chain_id;

    bool configured{false};
    bool wipe_database_on_startup{false};

    mongocxx::instance mongo_inst;
    mongocxx::client   mongo_conn;
    mongocxx::database mongo_db;

    evt_interpreter    interpreter;

    size_t                            queue_size = 0;
    size_t                            processed  = 0;
    std::deque<inblock_ptr>           block_state_queue;
    std::deque<transaction_trace_ptr> transaction_trace_queue;

    boost::mutex              mtx;
    boost::condition_variable condition;
    boost::thread             consume_thread;
    boost::atomic<bool>       done{false};
    boost::atomic<bool>       startup{true};

    channels::accepted_block::channel_type::handle      accepted_block_subscription;
    channels::irreversible_block::channel_type::handle  irreversible_block_subscription;
    channels::applied_transaction::channel_type::handle applied_transaction_subscription;

public:
    const std::string blocks_col        = "Blocks";
    const std::string trans_col         = "Transactions";
    const std::string actions_col       = "Actions";
    const std::string action_traces_col = "ActionTraces";
    const std::string domains_col       = "Domains";
    const std::string tokens_col        = "Tokens";
    const std::string groups_col        = "Groups";
    const std::string fungibles_col     = "Fungibles";
};

void
mongo_db_plugin_impl::applied_irreversible_block(const block_state_ptr& bsp) {
    try {
        boost::mutex::scoped_lock lock(mtx);
        block_state_queue.push_back(std::make_tuple(bsp, true));
        lock.unlock();
        condition.notify_one();
    }
    catch(fc::exception& e) {
        elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while applied_irreversible_block");
    }
}

void
mongo_db_plugin_impl::applied_block(const block_state_ptr& bsp) {
    try {
        boost::mutex::scoped_lock lock(mtx);
        block_state_queue.emplace_back(std::make_tuple(bsp, false));
        lock.unlock();
        condition.notify_one();
    }
    catch(fc::exception& e) {
        elog("FC Exception while applied_block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while applied_block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while applied_block");
    }
}

void
mongo_db_plugin_impl::applied_transaction(const transaction_trace_ptr& ttp) {
    try {
        if(startup) {
            // on startup we don't want to queue, instead push back on caller
            process_transaction(*ttp);
        }
        else {
            boost::mutex::scoped_lock lock(mtx);
            transaction_trace_queue.emplace_back(ttp);
            lock.unlock();
            condition.notify_one();
        }
    }
    catch(fc::exception& e) {
        elog("FC Exception while applied_transaction ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while applied_transaction ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while applied_transaction");
    }
}

void
mongo_db_plugin_impl::consume_queues() {
    try {
        while(true) {
            boost::mutex::scoped_lock lock(mtx);
            while(block_state_queue.empty() && transaction_trace_queue.empty() && !done) {
                condition.wait(lock);
            }
            // capture blocks for processing
            std::deque<inblock_ptr>           bqueue;
            std::deque<transaction_trace_ptr> tqueue;
            auto bsize = block_state_queue.size();
            if(bsize > 0) {
                bqueue = move(block_state_queue);
                block_state_queue.clear();
            }
            auto tsize = transaction_trace_queue.size();
            if(tsize > 0) {
                tqueue = move(transaction_trace_queue);
                transaction_trace_queue.clear();
            }
            lock.unlock();

            // warn if queue size greater than 75%
            if(bsize > (queue_size * 0.75) || tsize > (queue_size * 0.75)) {
                wlog("queue size: ${q}", ("q", bsize + tsize + 1));
            }
            else if(done) {
                ilog("draining queue, size: ${q}", ("q", bsize + tsize + 1));
            }

            const int BlockPtr = 0;
            const int IsIrreversible = 1;
            // process block states
            while(!bqueue.empty()) {
                const auto& b = bqueue.front();
                if(std::get<IsIrreversible>(b)) {
                    process_irreversible_block(*(std::get<BlockPtr>(b)->block));
                }
                else {
                    process_block(*(std::get<BlockPtr>(b)->block));
                }
                bqueue.pop_front();
            }

            // process transaction traces
            while(!tqueue.empty()) {
                const auto& t = tqueue.front();
                process_transaction(*t);
                tqueue.pop_front();
            }

            if(bsize == 0 && tsize == 0 && done) {
                break;
            }
        }
        ilog("mongo_db_plugin consume thread shutdown gracefully");
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

const auto&
get_find_options() {
    using bsoncxx::builder::stream::document;

    static mongocxx::options::find opts;
    static std::once_flag flag;
    static document project;

    std::call_once(flag, [&] {
        project << "_id" << 1;
        opts.projection(project.view());
    });

    return opts;
}

auto
find_transaction(mongocxx::collection& transactions, const string& id) {
    using bsoncxx::builder::stream::document;

    document project{};


    document find_trans{};
    find_trans << "trx_id" << id;
    auto transaction = transactions.find_one(find_trans.view(), get_find_options());
    if(!transaction) {
        FC_THROW("Unable to find transaction ${id}", ("id", id));
    }
    return *transaction;
}

auto
find_block(mongocxx::collection& blocks, const string& id) {
    using bsoncxx::builder::stream::document;
    document find_block{};
    find_block << "block_id" << id;
    auto block = blocks.find_one(find_block.view(), get_find_options());
    if(!block) {
        FC_THROW("Unable to find block ${id}", ("id", id));
    }
    return *block;
}

void
add_data(bsoncxx::builder::basic::document& msg_doc,
         const chain::action&               msg,
         const abi_serializer&              evt_abi) {
    using bsoncxx::builder::basic::kvp;
    try {
        auto& abis = evt_abi;
        auto v     = abis.binary_to_variant(abis.get_action_type(msg.name), msg.data);
        auto json  = fc::json::to_string(v);
        try {
            const auto& value = bsoncxx::from_json(json);
            msg_doc.append(kvp("data", value));
            return;
        }
        catch(std::exception& e) {
            elog("Unable to convert EVT JSON to MongoDB JSON: ${e}", ("e", e.what()));
            elog("  EVT JSON: ${j}", ("j", json));
        }
    }
    catch(fc::exception& e) {
        elog("Unable to convert action.data to ABI: ${n}, what: ${e}", ("n", msg.name)("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("Unable to convert action.data to ABI: ${n}, std what: ${e}", ("n", msg.name)("e", e.what()));
    }
    catch(...) {
        elog("Unable to convert action.data to ABI: ${n}, unknown exception", ("n", msg.name));
    }
    // if anything went wrong just store raw hex_data
    msg_doc.append(kvp("hex_data", fc::variant(msg.data).as_string()));
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
mongo_db_plugin_impl::process_irreversible_block(const signed_block& block) {
    try {
        if(block.block_num() == 1) {
            // genesis block will not trigger on_block event
            // add it manually
            _process_block(block); 
        }
        _process_irreversible_block(block);
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
mongo_db_plugin_impl::process_block(const signed_block& block) {
    try {
        _process_block(block);
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
mongo_db_plugin_impl::process_transaction(const transaction_trace& trace) {
    try {
        if(trace.except) {
            // failed transaction
            return;
        }
        _process_transaction(trace);
        interpreter.process_trx(trace);
    }
    catch(fc::exception& e) {
        elog("FC Exception while processing transaction trace ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("STD Exception while processing transaction trace ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while processing transaction block");
    }
}

void
mongo_db_plugin_impl::_process_block(const signed_block& block) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    mongocxx::options::bulk_write bulk_opts;
    bulk_opts.ordered(false);

    auto blocks        = mongo_db[blocks_col];         // Blocks
    auto trans         = mongo_db[trans_col];          // Transactions
    auto actions       = mongo_db[actions_col];        // Actions

    auto bulk_trans = trans.create_bulk_write(bulk_opts);
    auto bulk_acts  = actions.create_bulk_write(bulk_opts);

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

    block_doc.append(kvp("block_num", b_int32{block_num}),
                     kvp("block_id", block_id_str),
                     kvp("prev_block_id", prev_block_id_str),
                     kvp("timestamp", b_date{std::chrono::milliseconds{
                                          std::chrono::seconds{block.timestamp.operator fc::time_point().sec_since_epoch()}}}),
                     kvp("trx_merkle_root", block.transaction_mroot.str()),
                     kvp("trx_count", b_int32{(int)block.transactions.size()}),
                     kvp("producer", block.producer.to_string()),
                     kvp("pending", b_bool{true}));
    block_doc.append(kvp("created_at", b_date{now}));

    if(!blocks.insert_one(block_doc.view())) {
        elog("Failed to insert block ${bid}", ("bid", block_id));
    }

    int32_t act_num          = 0;
    bool    actions_to_write = false;
    auto    process_action   = [&](const std::string& trans_id_str, const chain::action& msg) -> auto {
        auto msg_oid = bsoncxx::oid{};
        auto msg_doc = bsoncxx::builder::basic::document{};
        msg_doc.append(kvp("_id", b_oid{msg_oid}),
                       kvp("trx_id", trans_id_str),
                       kvp("seq_num", b_int32{act_num}));
        msg_doc.append(kvp("name", msg.name.to_string()));
        msg_doc.append(kvp("domain", msg.domain.to_string()));
        msg_doc.append(kvp("key", msg.key.to_string()));
        add_data(msg_doc, msg, evt_abi);
        msg_doc.append(kvp("created_at", b_date{now}));
        mongocxx::model::insert_one insert_msg{msg_doc.view()};
        bulk_acts.append(insert_msg);
        actions_to_write = true;
        return msg_oid;
    };

    int32_t trx_num = 0;
    bool    transactions_in_block = false;

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
                   kvp("action_count", b_int32{(int)trx.actions.size()}),
                   kvp("expiration",
                       b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}),
                   kvp("pending", b_bool{true}));
        doc.append(kvp("created_at", b_date{now}));

        if(status == transaction_receipt_header::executed && !trx.actions.empty()) {
            act_num = 0;
            for(const auto& act : trx.actions) {
                process_action(trans_id_str, act);
                act_num++;
            }
        }
        transactions_in_block = true;
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

        mongocxx::model::insert_one insert_op{doc.view()};
        bulk_trans.append(insert_op);
        ++trx_num;
    }

    if(actions_to_write) {
        auto result = bulk_acts.execute();
        if(!result) {
            elog("Bulk actions insert failed for block: ${bid}", ("bid", block_id));
        }
    }
    if(transactions_in_block) {
        auto result = bulk_trans.execute();
        if(!result) {
            elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
        }
    }

    ++processed;
}

void
mongo_db_plugin_impl::_process_irreversible_block(const signed_block& block) {
    using namespace evt::__internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto blocks = mongo_db[blocks_col];  // Blocks
    auto trans  = mongo_db[trans_col];   // Transactions

    const auto block_id     = block.id();
    const auto block_id_str = block_id.str();

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto ir_block = find_block(blocks, block_id_str);

    document update_block{};
    update_block << "$set" << open_document << "pending" << b_bool{false}
                 << "updated_at" << b_date{now}
                 << close_document;

    blocks.update_one(document{} << "_id" << ir_block.view()["_id"].get_oid() << finalize, update_block.view());

    for(const auto& trx_receipt : block.transactions) {
        auto trans_id_str = trx_receipt.trx.id().str();
        auto ir_trans     = find_transaction(trans, trans_id_str);

        document update_trans{};
        update_trans << "$set" << open_document << "pending" << b_bool{false}
                     << "updated_at" << b_date{now}
                     << close_document;

        trans.update_one(document{} << "_id" << ir_trans.view()["_id"].get_oid() << finalize,
                         update_trans.view());
    }
}

void
mongo_db_plugin_impl::_process_transaction(const transaction_trace& trace) {
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto action_traces          = mongo_db[action_traces_col];  // ActionTraces
    bool action_traces_to_write = false;
    auto seq_num                = 0;
    auto process_action_trace   = [&](const std::string&       trans_id_str,
                                    mongocxx::bulk_write&      bulk_acts,
                                    const chain::action_trace& act) {
        auto act_oid = bsoncxx::oid{};
        auto act_doc = bsoncxx::builder::basic::document{};
        act_doc.append(kvp("_id", b_oid{act_oid}),
                       kvp("trx_id", trans_id_str),
                       kvp("elapsed", act.elapsed.count()),
                       kvp("seq_num", b_int32{seq_num}),
                       kvp("console", act.console));
        act_doc.append(kvp("created_at", b_date{now}));
        mongocxx::model::insert_one insert_act{act_doc.view()};
        bulk_acts.append(insert_act);
        action_traces_to_write = true;
    };

    auto trx_id_str = trace.id.str();

    mongocxx::options::bulk_write bulk_opts;
    bulk_opts.ordered(false);

    auto bulk_acts  = action_traces.create_bulk_write(bulk_opts);
    for(auto& at : trace.action_traces) {
        process_action_trace(trx_id_str, bulk_acts, at);
        seq_num++;
    }

    if(action_traces_to_write) {
        auto result = bulk_acts.execute();
        if(!result) {
            elog("Bulk action traces insert failed for transaction: ${tid}", ("tid", trx_id_str));
        }
    }
}

mongo_db_plugin_impl::~mongo_db_plugin_impl() {
    try {
        done = true;
        condition.notify_one();

        consume_thread.join();
    }
    catch(std::exception& e) {
        elog("Exception on mongo_db_plugin shutdown of consume thread: ${e}", ("e", e.what()));
    }
}

void
mongo_db_plugin_impl::wipe_database() {
    ilog("mongo db wipe_database");

    auto blocks        = mongo_db[blocks_col];         // Blocks
    auto trans         = mongo_db[trans_col];          // Transactions
    auto msgs          = mongo_db[actions_col];        // Actions
    auto action_traces = mongo_db[action_traces_col];  // ActionTraces
    auto domains       = mongo_db[domains_col];
    auto tokens        = mongo_db[tokens_col];
    auto groups        = mongo_db[groups_col];
    auto fungibles     = mongo_db[fungibles_col];

    blocks.drop();
    trans.drop();
    msgs.drop();
    action_traces.drop();
    domains.drop();
    tokens.drop();
    groups.drop();
    fungibles.drop();
}

void
mongo_db_plugin_impl::init() {
    using namespace bsoncxx::types;

    auto blocks = mongo_db[blocks_col];  // Blocks
    bsoncxx::builder::stream::document doc{};
    if(blocks.count(doc.view()) == 0) {
        // Blocks indexes
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

        // Transactions indexes
        auto trans = mongo_db[trans_col];  // Transactions
        trans.create_index(bsoncxx::from_json(R"xxx({ "trx_id" : 1 })xxx"));

        // Action indexes
        auto acts = mongo_db[actions_col];  // Actions
        acts.create_index(bsoncxx::from_json(R"xxx({ "domain" : 1 })xxx"));
        acts.create_index(bsoncxx::from_json(R"xxx({ "trx_id" : 1 })xxx"));

        // ActionTraces indexes
        auto action_traces = mongo_db[action_traces_col];  // ActionTraces
        action_traces.create_index(bsoncxx::from_json(R"xxx({ "trx_id" : 1 })xxx"));

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
        fungibles.create_index(bsoncxx::from_json(R"xxx({ "sym" : 1 })xxx"));
    }

    // initilize evt interpreter
    interpreter.initialize_db(mongo_db);

    // connect callbacks to channel
    accepted_block_subscription = app().get_channel<channels::accepted_block>().subscribe(
        boost::bind(&mongo_db_plugin_impl::applied_block, this, _1));
    irreversible_block_subscription = app().get_channel<channels::irreversible_block>().subscribe(
        boost::bind(&mongo_db_plugin_impl::applied_irreversible_block, this, _1));
    applied_transaction_subscription = app().get_channel<channels::applied_transaction>().subscribe(
        boost::bind(&mongo_db_plugin_impl::applied_transaction, this, _1));
}

////////////
// mongo_db_plugin
////////////

mongo_db_plugin::mongo_db_plugin()
    : my_(new mongo_db_plugin_impl) {
}

mongo_db_plugin::~mongo_db_plugin() {
}

const mongocxx::database&
mongo_db_plugin::db() const {
    return my_->mongo_db;
}

void
mongo_db_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("mongodb-queue-size,q", bpo::value<uint>()->default_value(256), "The queue size between evtd and MongoDB plugin thread.")
        ("mongodb-uri,m", bpo::value<std::string>(), "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/."
                                                     " If not specified then plugin is disabled. Default database 'EVT' is used if not specified in URI.")
        ;
}

void
mongo_db_plugin::plugin_initialize(const variables_map& options) {
    if(options.count("mongodb-uri")) {
        ilog("initializing mongo_db_plugin");
        my_->configured = true;

        if(options.at("replay-blockchain").as<bool>() || options.at("hard-replay-blockchain").as<bool>()) {
            ilog("Replay requested: wiping mongo database on startup");
            my_->wipe_database_on_startup = true;
        } 
        if(options.at("delete-all-blocks").as<bool>()) {
            ilog("Deleted all blocks: wiping mongo database on startup");
            my_->wipe_database_on_startup = true;
        }
        if(options.count("mongodb-queue-size")) {
            auto size      = options.at("mongodb-queue-size").as<uint>();
            my_->queue_size = size;
        }

        std::string uri_str = options.at("mongodb-uri").as<std::string>();
        ilog("connecting to ${u}", ("u", uri_str));
        
        auto uri    = mongocxx::uri{uri_str};
        auto dbname = uri.database();
        if(dbname.empty())
            dbname = "EVT";

        my_->mongo_conn = mongocxx::client{uri};
        my_->mongo_db   = my_->mongo_conn[dbname];

        my_->evt_abi  = evt_contract_abi();
        my_->chain_id = app().get_plugin<chain_plugin>().chain().get_chain_id();

        if(my_->wipe_database_on_startup) {
            my_->wipe_database();
        }
        my_->init();
    }
    else {
        wlog("evt::mongo_db_plugin configured, but no --mongodb-uri specified.");
        wlog("mongo_db_plugin disabled.");
    }
}

void
mongo_db_plugin::plugin_startup() {
    if(my_->configured) {
        ilog("starting db plugin");

        my_->consume_thread = boost::thread([this] { my_->consume_queues(); });

        // chain_controller is created and has resynced or replayed if needed
        my_->startup = false;
    }
}

void
mongo_db_plugin::plugin_shutdown() {
    my_.reset();
}

}  // namespace evt
