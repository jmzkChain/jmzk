/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/postgres_plugin/postgres_plugin.hpp>

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
#include <evt/chain/exceptions.hpp>
#include <evt/chain/genesis_state.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/utilities/spinlock.hpp>

#include <evt/postgres_plugin/evt_pg.hpp>
#include <evt/postgres_plugin/copy_context.hpp>
#include <evt/postgres_plugin/trx_context.hpp>

namespace evt {

using namespace chain;
using namespace chain::contracts;
using namespace chain::plugin_interface;

using evt::utilities::spinlock;
using evt::utilities::spinlock_guard;

static appbase::abstract_plugin& _postgres_plugin = app().register_plugin<postgres_plugin>();

class postgres_plugin_impl {
private:
    using inblock_ptr = std::tuple<block_state_ptr, bool>; // true for irreversible block

public:
    postgres_plugin_impl()
        : abi_(evt_contract_abi(), fc::hours(1))
    { }

    ~postgres_plugin_impl();

public:
    void consume_queues();

    void applied_block(const block_state_ptr&);
    void applied_irreversible_block(const block_state_ptr&);
    void applied_transaction(const transaction_trace_ptr&);

    void process_block(const block_state_ptr, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx);
    void _process_block(const block_state_ptr, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx);
    void process_irreversible_block(const block_state_ptr, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx);
    
    void process_action(const action&, trx_context& tctx);

    void verify_last_block(const std::string& prev_block_id);
    void verify_no_blocks();

    void init(const std::string& name);
    void wipe_database(const std::string& name);

public:
    pg db_;

    abi_serializer              abi_;
    fc::optional<chain_id_type> chain_id_;

    bool configured_               = false;
    bool wipe_database_on_startup_ = false;

    size_t processed_  = 0;
    size_t queue_size_ = 0;

    std::deque<inblock_ptr>           block_state_queue_;
    std::deque<transaction_trace_ptr> transaction_trace_queue_;

    spinlock                    lock_;
    condition_variable_any      cond_;
    std::thread                 consume_thread_;
    std::atomic_bool            done_ = false;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection_;
    fc::optional<boost::signals2::scoped_connection> irreversible_block_connection_;
    fc::optional<boost::signals2::scoped_connection> applied_transaction_connection_;
};

namespace __internal {

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

}  // namespace __internal

void
postgres_plugin_impl::applied_irreversible_block(const block_state_ptr& bsp) {
    evt::__internal::queueb(block_state_queue_, std::make_tuple(bsp, true), lock_, cond_, queue_size_);
}

void
postgres_plugin_impl::applied_block(const block_state_ptr& bsp) {
    evt::__internal::queueb(block_state_queue_, std::make_tuple(bsp, false), lock_, cond_, queue_size_);
}

void
postgres_plugin_impl::applied_transaction(const transaction_trace_ptr& ttp) {
    evt::__internal::queuet(transaction_trace_queue_, ttp, lock_, cond_);
}

void
postgres_plugin_impl::consume_queues() {
    using namespace evt::__internal;

    try {
        while(true) {
            lock_.lock();
            while(block_state_queue_.empty() && !done_) {
                cond_.wait(lock_);
            }

            auto bqueue = std::move(block_state_queue_);
            auto traces = std::move(transaction_trace_queue_);

            lock_.unlock();

            const int BlockPtr       = 0;
            const int IsIrreversible = 1;

            // warn if queue size greater than 75%
            if(bqueue.size() > (queue_size_ * 0.75)) {
                wlog("queue size: ${q}, head block num: ${b}", ("q", bqueue.size())("b",std::get<BlockPtr>(bqueue.front())->block_num));
            }
            else if(done_) {
                ilog("draining queue, size: ${q}", ("q", bqueue.size()));
                break;
            }

            auto cctx = db_.new_copy_context();
            auto tctx = db_.new_trx_context();
            // process block states
            while(true) {
                if(bqueue.empty()) {
                    break;
                }

                auto& b = bqueue.front();
                if(std::get<IsIrreversible>(b)) {
                    process_irreversible_block(std::get<BlockPtr>(b), traces, cctx, tctx);
                }
                else {
                    process_block(std::get<BlockPtr>(b), traces, cctx, tctx);
                }

                bqueue.pop_front();
            }
            cctx.commit();
            tctx.commit();

            if(!traces.empty()) {
                spinlock_guard lock(lock_);
                transaction_trace_queue_.insert(transaction_trace_queue_.begin(), traces.begin(), traces.end());
            }
        }
        ilog("postgres_plugin consume thread shutdown gracefully");
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

void
postgres_plugin_impl::verify_last_block(const std::string& prev_block_id) {
    auto last_block_id = std::string();
    if(!db_.get_latest_block_id(last_block_id)) {
        EVT_THROW(postgresql_exception, "No blocks found in database");
    }

    EVT_ASSERT(prev_block_id == last_block_id, postgresql_exception,
        "Did not find expected block ${pid}, instead found ${id}", ("pid", prev_block_id)("id", last_block_id));
}

void
postgres_plugin_impl::verify_no_blocks() {
    if(!db_.is_table_empty("blocks")) {
        EVT_THROW(postgresql_exception, "Existing blocks found in database");
    }
}

void
postgres_plugin_impl::process_irreversible_block(const block_state_ptr block, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx) {
    try {
        if(block->block_num == 1) {
            // genesis block will not trigger on_block event
            // add it manually
            _process_block(block, traces, cctx, tctx);
        }
        db_.set_block_irreversible(tctx, block->id);
    }
    catch(fc::exception& e) {
        elog("Exception while processing irreversible block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("Exception while processing irreversible block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while processing irreversible block");
    }
}

void
postgres_plugin_impl::process_block(const block_state_ptr block, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx) {
    try {
        _process_block(block, traces, cctx, tctx);
    }
    catch(fc::exception& e) {
        elog("Exception while processing block ${e}", ("e", e.to_string()));
    }
    catch(std::exception& e) {
        elog("Exception while processing block ${e}", ("e", e.what()));
    }
    catch(...) {
        elog("Unknown exception while processing block");
    }
}

#define case_act(act_name, func_name)                        \
    case N(act_name): {                                      \
        db_.func_name(tctx, act.data_as<const act_name&>()); \
        break;                                               \
    }

void
postgres_plugin_impl::process_action(const action& act, trx_context& tctx) {
    switch((uint64_t)act.name) {
    case_act(newdomain,    add_domain);
    case_act(updatedomain, upd_domain);
    case_act(issuetoken,   add_tokens);
    case_act(transfer,     upd_token);
    case_act(destroytoken, del_token);
    case_act(newgroup,     add_group);
    case_act(updategroup,  upd_group);
    case_act(newfungible,  add_fungible);
    case_act(updfungible,  upd_fungible);
    }; // switch
}

void
postgres_plugin_impl::_process_block(const block_state_ptr block, std::deque<transaction_trace_ptr>& traces, copy_context& cctx, trx_context& tctx) {
    using namespace evt::__internal;

    if(processed_ == 0) {
        if(block->block_num <= 2) {
            // verify on start we have no previous blocks
            verify_no_blocks();
        }
        else {
            // verify on restart we have previous block
            verify_last_block(block->header.previous.str());
        }
    }

    auto actx      = add_context(cctx, *chain_id_, abi_);
    actx.block_id  = block->id.str();
    actx.block_num = (int)block->block_num;
    actx.ts        = (std::string)block->header.timestamp.to_time_point();

    db_.add_block(actx, block);

    auto block_id  = block->id.str();
    auto block_num = block->block_num;
    auto ts        = (std::string)block->header.timestamp.to_time_point(); 

    // transactions
    auto trx_num = 0;
    for(const auto& trx : block->block->transactions) {
        auto strx    = trx.trx.get_signed_transaction();
        auto trx_id  = strx.id();
        auto elapsed = 0;
        auto charge  = 0;

        if(trx.status == transaction_receipt_header::executed && !strx.actions.empty()) {
            auto act_num = 0;
            for(const auto& act : strx.actions) {
                db_.add_action(actx, act, trx_id, act_num);
                process_action(act, tctx);
                act_num++;
            }

            auto it = traces.begin();
            while(it != traces.end()) {
                auto trace = *it;
                traces.pop_front();

                if(trace->id == trx_id) {
                    elapsed = (int)trace->elapsed.count();
                    charge  = (int)trace->charge;

                    if(trace->action_traces.empty()) {
                        break;
                    }
                    // because paycharage action is alwasys the latest action
                    // only check latest
                    auto& act = trace->action_traces.back().act;
                    if(act.name == N(paycharge)) {
                        db_.add_action(actx, act, trx_id, act_num);
                    }
                    break;
                }
                it++;
            }
        }

        db_.add_trx(actx, trx, strx, trx_num, elapsed, charge);
        ++trx_num;
    }

    ++processed_;
}

void
postgres_plugin_impl::wipe_database(const std::string& name) {
    ilog("wipe database");
    if(db_.exists_db(name)) {
        db_.drop_table("blocks");
        db_.drop_table("transactions");
        db_.drop_table("actions");
        db_.drop_table("domains");
        db_.drop_table("tokens");
        db_.drop_table("groups");
        db_.drop_table("fungibles");
    }
}

void
postgres_plugin_impl::init(const std::string& name) {
    if(!db_.exists_db(name)) {
        db_.create_db(name);
    }
    db_.prepare_tables();

    auto need_init = db_.is_table_empty("blocks");

    // initilize evt interpreter
    // interpreter.initialize_db();

    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection_.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));

    irreversible_block_connection_.emplace(chain.irreversible_block.connect([&](const chain::block_state_ptr& bs) {
        applied_irreversible_block(bs);
    }));

    applied_transaction_connection_.emplace(chain.applied_transaction.connect([&](const chain::transaction_trace_ptr& t) {
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

        auto tctx = db_.new_trx_context();
        process_action(get_nfact(gs.evt), tctx);
        process_action(get_nfact(gs.pevt), tctx);
        process_action(action(N128(.group), N128(.everiToken), ng), tctx);

        tctx.commit();
    }
}

postgres_plugin_impl::~postgres_plugin_impl() {
    if(!configured_) {
        return;
    }
    try {
        done_ = true;
        cond_.notify_one();

        consume_thread_.join();
    }
    catch(std::exception& e) {
        elog("Exception on postgres_plugin shutdown of consume thread: ${e}", ("e", e.what()));
    }
}

postgres_plugin::postgres_plugin()
    : my_(new postgres_plugin_impl()) {
}

postgres_plugin::~postgres_plugin() {}

bool
postgres_plugin::enabled() const {
    return my_->configured_;
}

void
postgres_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("postgres-queue-size,q", bpo::value<uint>()->default_value(5120), "The queue size between evtd and postgres plugin thread.")
        ("postgres-uri,p", bpo::value<std::string>(), "PostgreSQL connection string, see: https://www.postgresql.org/docs/11/libpq-connect.html#LIBPQ-CONNSTRING for more detail.")
        ("postgres-db", bpo::value<std::string>()->default_value("evt"), "Database name in postgres");
        ;
}

void
postgres_plugin::plugin_initialize(const variables_map& options) {
    if(options.count("postgres-uri")) {
        ilog("initializing postgres_plugin");
        my_->configured_ = true;

        if(options.at("replay-blockchain").as<bool>() || options.at("hard-replay-blockchain").as<bool>()) {
            ilog("Replay requested: wiping postgres database on startup");
            my_->wipe_database_on_startup_ = true;
        } 
        if(options.at("delete-all-blocks").as<bool>()) {
            ilog("Deleted all blocks: wiping postgres database on startup");
            my_->wipe_database_on_startup_ = true;
        }
        if(options.count("import-reversible-blocks")) {
            ilog("Importing reversible blocks: wiping postgres database on startup");
            my_->wipe_database_on_startup_ = true;
        }
        
        if(options.count("postgres-queue-size")) {
            my_->queue_size_ = options.at("postgres-queue-size").as<uint>();
        }

        auto uri = options.at("postgres-uri").as<std::string>();
        ilog("connecting to ${u}", ("u", uri));
        
        auto dbname = options.at("postgres-db").as<std::string>();

        my_->db_.connect(uri);
        my_->chain_id_ = app().get_plugin<chain_plugin>().chain().get_chain_id();

        if(my_->wipe_database_on_startup_) {
            my_->wipe_database(dbname);
        }

        my_->init(dbname);

        my_->consume_thread_ = std::thread([this] { my_->consume_queues(); });
    }
    else {
        wlog("evt::postgres_plugin configured, but no --postgres-uri specified.");
        wlog("postgres_plugin disabled.");
    }
}

void
postgres_plugin::plugin_startup() {}

void
postgres_plugin::plugin_shutdown() {
    my_->accepted_block_connection_.reset();
    my_->irreversible_block_connection_.reset();
    my_->applied_transaction_connection_.reset();
    my_.reset();
}

}  // namespace evt
