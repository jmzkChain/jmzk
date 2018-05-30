/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/mongo_db_plugin/mongo_db_plugin.hpp>
#include <evt/mongo_db_plugin/evt_interpreter.hpp>
#include <evt/mongo_db_plugin/wallet_query.hpp>

#include <queue>

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
public:
    mongo_db_plugin_impl();
    ~mongo_db_plugin_impl();

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

    static abi_def eos_abi;  // cached for common use
    abi_serializer evt_api;

    bool configured{false};
    bool wipe_database_on_startup{false};

    std::string        db_name;
    mongocxx::instance mongo_inst;
    mongocxx::client   mongo_conn;

    evt_interpreter    interpreter;

    size_t                            queue_size = 0;
    size_t                            processed  = 0;
    std::deque<block_state_ptr>       block_state_queue;
    std::deque<transaction_trace_ptr> transaction_trace_queue;

    boost::mutex              mtx;
    boost::condition_variable condition;
    boost::thread             consume_thread;
    boost::atomic<bool>       done{false};
    boost::atomic<bool>       startup{true};

    channels::accepted_block::channel_type::handle      accepted_block_subscription;
    channels::irreversible_block::channel_type::handle  irreversible_block_subscription;
    channels::applied_transaction::channel_type::handle applied_transaction_subscription;

    void consume_queues();

    static const std::string blocks_col;
    static const std::string trans_col;
    static const std::string actions_col;
    static const std::string action_traces_col;
    static const std::string domains_col;
    static const std::string tokens_col;
    static const std::string groups_col;
    static const std::string accounts_col;
};

const std::string mongo_db_plugin_impl::blocks_col        = "Blocks";
const std::string mongo_db_plugin_impl::trans_col         = "Transactions";
const std::string mongo_db_plugin_impl::actions_col       = "Actions";
const std::string mongo_db_plugin_impl::action_traces_col = "ActionTraces";
const std::string mongo_db_plugin_impl::domains_col       = "Domains";
const std::string mongo_db_plugin_impl::tokens_col        = "Tokens";
const std::string mongo_db_plugin_impl::groups_col        = "Groups";
const std::string mongo_db_plugin_impl::accounts_col      = "Accounts";

void
mongo_db_plugin_impl::applied_irreversible_block(const block_state_ptr& bsp) {
    try {
        if(startup) {
            // on startup we don't want to queue, instead push back on caller
            process_irreversible_block(*bsp->block);
        }
        else {
            boost::mutex::scoped_lock lock(mtx);
            block_state_queue.push_back(bsp);
            lock.unlock();
            condition.notify_one();
        }
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
        if(startup) {
            // on startup we don't want to queue, instead push back on caller
            process_block(*bsp->block);
        }
        else {
            boost::mutex::scoped_lock lock(mtx);
            block_state_queue.emplace_back(bsp);
            lock.unlock();
            condition.notify_one();
        }
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
            std::deque<block_state_ptr>       bqueue;
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

            // process block states
            while(!bqueue.empty()) {
                const auto& b = bqueue.front();
                if(b->bft_irreversible_blocknum > 0) {
                    process_irreversible_block(*b->block);
                }
                else {
                    process_block(*b->block);
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

auto
find_transaction(mongocxx::collection& transactions, const string& id) {
    using bsoncxx::builder::stream::document;
    document find_trans{};
    find_trans << "transaction_id" << id;
    auto transaction = transactions.find_one(find_trans.view());
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
    auto block = blocks.find_one(find_block.view());
    if(!block) {
        FC_THROW("Unable to find block ${id}", ("id", id));
    }
    return *block;
}

void
add_data(bsoncxx::builder::basic::document& msg_doc,
         const chain::action&               msg,
         const abi_serializer&              evt_api) {
    using bsoncxx::builder::basic::kvp;
    try {
        auto& abis = evt_api;
        auto v     = abis.binary_to_variant(abis.get_action_type(msg.name), msg.data);
        auto json  = fc::json::to_string(v);
        try {
            const auto& value = bsoncxx::from_json(json);
            msg_doc.append(kvp("data", value));
            return;
        }
        catch(std::exception& e) {
            elog("Unable to convert EOS JSON to MongoDB JSON: ${e}", ("e", e.what()));
            elog("  EOS JSON: ${j}", ("j", json));
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
    opts.sort(bsoncxx::from_json(R"xxx({ "_id" : -1 })xxx"));
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
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;

    mongocxx::options::bulk_write bulk_opts;
    bulk_opts.ordered(false);
    mongocxx::bulk_write bulk_trans{bulk_opts};

    auto blocks        = mongo_conn[db_name][blocks_col];         // Blocks
    auto trans         = mongo_conn[db_name][trans_col];          // Transactions
    auto msgs          = mongo_conn[db_name][actions_col];        // Actions

    auto       block_doc         = bsoncxx::builder::basic::document{};
    const auto block_id          = block.id();
    const auto block_id_str      = block_id.str();
    const auto prev_block_id_str = block.previous.str();
    auto       block_num         = block.block_num();

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

    block_doc.append(kvp("block_num", b_int32{static_cast<int32_t>(block_num)}),
                     kvp("block_id", block_id_str),
                     kvp("prev_block_id", prev_block_id_str),
                     kvp("timestamp", b_date{std::chrono::milliseconds{
                                          std::chrono::seconds{block.timestamp.operator fc::time_point().sec_since_epoch()}}}),
                     kvp("transaction_merkle_root", block.transaction_mroot.str()),
                     kvp("producer_account_id", block.producer.to_string()),
                     kvp("pending", b_bool{true}));
    block_doc.append(kvp("createdAt", b_date{now}));

    if(!blocks.insert_one(block_doc.view())) {
        elog("Failed to insert block ${bid}", ("bid", block_id));
    }

    int32_t msg_num          = -1;
    bool    actions_to_write = false;
    auto    process_action   = [&](const std::string& trans_id_str, mongocxx::bulk_write& bulk_msgs, const chain::action& msg) -> auto {
        auto msg_oid = bsoncxx::oid{};
        auto msg_doc = bsoncxx::builder::basic::document{};
        msg_doc.append(kvp("_id", b_oid{msg_oid}),
                       kvp("action_id", b_int32{msg_num}),
                       kvp("transaction_id", trans_id_str));
        msg_doc.append(kvp("name", msg.name.to_string()));
        msg_doc.append(kvp("domain", msg.domain.to_string()));
        msg_doc.append(kvp("key", msg.key.to_string()));

        add_data(msg_doc, msg, evt_api);
        msg_doc.append(kvp("createdAt", b_date{now}));
        mongocxx::model::insert_one insert_msg{msg_doc.view()};
        bulk_msgs.append(insert_msg);
        actions_to_write = true;
        ++msg_num;
        return msg_oid;
    };

    int32_t                                           trx_num = 0;
    std::map<chain::transaction_id_type, std::string> trx_status_map;
    bool                                              transactions_in_block = false;

    auto process_trx = [&](const chain::transaction& trx) -> auto {
        auto       txn_oid      = bsoncxx::oid{};
        auto       doc          = bsoncxx::builder::basic::document{};
        auto       trx_id       = trx.id();
        const auto trans_id_str = trx_id.str();
        doc.append(kvp("_id", txn_oid),
                   kvp("transaction_id", trans_id_str),
                   kvp("sequence_num", b_int32{trx_num}),
                   kvp("block_id", block_id_str),
                   kvp("ref_block_num", b_int32{static_cast<int32_t>(trx.ref_block_num)}),
                   kvp("ref_block_prefix", b_int32{static_cast<int32_t>(trx.ref_block_prefix)}),
                   kvp("status", trx_status_map[trx_id]),
                   kvp("expiration",
                       b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}),
                   kvp("pending", b_bool{true}));
        doc.append(kvp("createdAt", b_date{now}));

        if(!trx.actions.empty()) {
            mongocxx::bulk_write bulk_msgs{bulk_opts};
            msg_num = 0;
            for(const auto& msg : trx.actions) {
                process_action(trans_id_str, bulk_msgs, msg);
            }
            auto result = msgs.bulk_write(bulk_msgs);
            if(!result) {
                elog("Bulk action insert failed for block: ${bid}, transaction: ${trx}",
                     ("bid", block_id)("trx", trx_id));
            }
        }
        transactions_in_block = true;
        return doc;
    };

    trx_num = 0;
    for(const auto& trx_receipt : block.transactions) {
        const auto& trx = trx_receipt.trx.get_signed_transaction();
        auto        doc = process_trx(trx);
        doc.append(kvp("type", "input"));
        doc.append(kvp("signatures", [&trx](bsoncxx::builder::basic::sub_array subarr) {
            for(const auto& sig : trx.signatures) {
                subarr.append(fc::variant(sig).as_string());
            }
        }));
        mongocxx::model::insert_one insert_op{doc.view()};
        bulk_trans.append(insert_op);
        ++trx_num;
    }

    if(transactions_in_block) {
        auto result = trans.bulk_write(bulk_trans);
        if(!result) {
            elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
        }
    }

    ++processed;
}

void
mongo_db_plugin_impl::_process_irreversible_block(const signed_block& block) {
    using namespace __internal;
    using namespace bsoncxx::types;
    using namespace bsoncxx::builder;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_document;

    auto blocks = mongo_conn[db_name][blocks_col];  // Blocks
    auto trans  = mongo_conn[db_name][trans_col];   // Transactions

    const auto block_id     = block.id();
    const auto block_id_str = block_id.str();

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

    auto ir_block = find_block(blocks, block_id_str);

    document update_block{};
    update_block << "$set" << open_document << "pending" << b_bool{false}
                 << "updatedAt" << b_date{now}
                 << close_document;

    blocks.update_one(document{} << "_id" << ir_block.view()["_id"].get_oid() << finalize, update_block.view());

    for(const auto& trx_receipt : block.transactions) {
        auto trans_id_str = trx_receipt.trx.id().str();
        auto ir_trans     = find_transaction(trans, trans_id_str);

        document update_trans{};
        update_trans << "$set" << open_document << "pending" << b_bool{false}
                     << "updatedAt" << b_date{now}
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

    auto action_traces          = mongo_conn[db_name][action_traces_col];  // ActionTraces
    bool action_traces_to_write = false;
    auto process_action_trace   = [&](const std::string&       trans_id_str,
                                    mongocxx::bulk_write&      bulk_acts,
                                    const chain::action_trace& act,
                                    const auto&                msg_oid) {
        auto act_oid = bsoncxx::oid{};
        auto act_doc = bsoncxx::builder::basic::document{};
        act_doc.append(kvp("_id", b_oid{act_oid}),
                       kvp("transaction_id", trans_id_str),
                       kvp("elapsed", act.elapsed.count()),
                       kvp("action", b_oid{msg_oid}),
                       kvp("console", act.console));
        act_doc.append(kvp("createdAt", b_date{now}));
        mongocxx::model::insert_one insert_act{act_doc.view()};
        bulk_acts.append(insert_act);
        action_traces_to_write = true;
    };

    auto trx_id_str = trace.id.str();
    auto bulk_acts  = mongocxx::bulk_write{};
    auto oid        = bsoncxx::oid{};

    for(auto& at : trace.action_traces) {
        process_action_trace(trx_id_str, bulk_acts, at, oid);
    }

    if(action_traces_to_write) {
        auto result = action_traces.bulk_write(bulk_acts);
        if(!result) {
            elog("Bulk action traces insert failed for transaction: ${tid}", ("tid", trx_id_str));
        }
    }
}

mongo_db_plugin_impl::mongo_db_plugin_impl()
    : mongo_inst{}
    , mongo_conn{} {
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

    auto blocks        = mongo_conn[db_name][blocks_col];         // Blocks
    auto trans         = mongo_conn[db_name][trans_col];          // Transactions
    auto msgs          = mongo_conn[db_name][actions_col];        // Actions
    auto action_traces = mongo_conn[db_name][action_traces_col];  // ActionTraces
    auto domains       = mongo_conn[db_name][domains_col];
    auto tokens        = mongo_conn[db_name][tokens_col];
    auto groups        = mongo_conn[db_name][groups_col];
    auto accounts      = mongo_conn[db_name][accounts_col];

    blocks.drop();
    trans.drop();
    msgs.drop();
    action_traces.drop();
    domains.drop();
    tokens.drop();
    groups.drop();
    accounts.drop();
}

void
mongo_db_plugin_impl::init() {
    using namespace bsoncxx::types;
    // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
    // See native_contract_chain_initializer::prepare_database()

    auto blocks = mongo_conn[db_name][blocks_col];  // Blocks
    bsoncxx::builder::stream::document doc{};
    if(blocks.count(doc.view()) == 0) {
        // Blocks indexes
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
        blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

        // Transactions indexes
        auto trans = mongo_conn[db_name][trans_col];  // Transactions
        trans.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

        // Action indexes
        auto acts = mongo_conn[db_name][actions_col];  // Actions
        acts.create_index(bsoncxx::from_json(R"xxx({ "action_id" : 1 })xxx"));
        acts.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

        // ActionTraces indexes
        auto action_traces = mongo_conn[db_name][action_traces_col];  // ActionTraces
        action_traces.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

        // Domains indexes
        auto domains = mongo_conn[db_name][domains_col];
        domains.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

        // Tokens indexes
        auto tokens = mongo_conn[db_name][tokens_col];
        tokens.create_index(bsoncxx::from_json(R"xxx({ "token_id" : 1 })xxx"));

        // Groups indexes
        auto groups = mongo_conn[db_name][groups_col];
        groups.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

        // Accounts indexes
        auto accounts = mongo_conn[db_name][accounts_col];
        accounts.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));
    }

    // initilize evt interpreter
    interpreter.initialize_db(mongo_conn[db_name]);

    evt_api = abi_serializer(contracts::evt_contract_abi());

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
    : my(new mongo_db_plugin_impl) {
}

mongo_db_plugin::~mongo_db_plugin() {
}

void
mongo_db_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("mongodb-queue-size,q", bpo::value<uint>()->default_value(256), "The queue size between evtd and MongoDB plugin thread.")
        ("mongodb-uri,m", bpo::value<std::string>(), "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/."
                                                     " If not specified then plugin is disabled. Default database 'EVT' is used if not specified in URI.");
}

void
mongo_db_plugin::plugin_initialize(const variables_map& options) {
    if(options.count("mongodb-uri")) {
        ilog("initializing mongo_db_plugin");
        my->configured = true;

        if(options.at("replay-blockchain").as<bool>()) {
            ilog("Replay requested: wiping mongo database on startup");
            my->wipe_database_on_startup = true;
        }

        if(options.count("mongodb-queue-size")) {
            auto size      = options.at("mongodb-queue-size").as<uint>();
            my->queue_size = size;
        }

        std::string uri_str = options.at("mongodb-uri").as<std::string>();
        ilog("connecting to ${u}", ("u", uri_str));
        mongocxx::uri uri = mongocxx::uri{uri_str};
        my->db_name       = uri.database();
        if(my->db_name.empty())
            my->db_name = "EVT";
        my->mongo_conn = mongocxx::client{uri};

        if(my->wipe_database_on_startup) {
            my->wipe_database();
        }
        my->init();
    }
    else {
        wlog("evt::mongo_db_plugin configured, but no --mongodb-uri specified.");
        wlog("mongo_db_plugin disabled.");
    }
}

void
mongo_db_plugin::plugin_startup() {
    if(my->configured) {
        ilog("starting db plugin");

        my->consume_thread = boost::thread([this] { my->consume_queues(); });

        // chain_controller is created and has resynced or replayed if needed
        my->startup = false;
    }
}

void
mongo_db_plugin::plugin_shutdown() {
    my.reset();
}

fc::flat_set<std::string>
mongo_db_plugin::get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys) {
    auto query = wallet_query(my->mongo_conn[my->db_name]);
    return query.get_tokens_by_public_keys(pkeys);
}

fc::flat_set<std::string>
mongo_db_plugin::get_domains_by_public_keys(const std::vector<public_key_type>& pkeys) {
    auto query = wallet_query(my->mongo_conn[my->db_name]);
    return query.get_domains_by_public_keys(pkeys);
}

fc::flat_set<std::string>
mongo_db_plugin::get_groups_by_public_keys(const std::vector<public_key_type>& pkeys) {
    auto query = wallet_query(my->mongo_conn[my->db_name]);
    return query.get_domains_by_public_keys(pkeys);
}

}  // namespace evt
