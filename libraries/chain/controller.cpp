/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/controller.hpp>
#include <evt/chain/transaction_context.hpp>

#include <evt/chain/authority_checker.hpp>
#include <evt/chain/block_log.hpp>
#include <evt/chain/fork_database.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/charge_manager.hpp>

#include <evt/chain/block_summary_object.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/transaction_object.hpp>
#include <evt/chain/reversible_block_object.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>
#include <fc/scoped_exit.hpp>

#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/evt_org.hpp>

namespace evt { namespace chain {

struct pending_state {
    pending_state(database::session&& s, token_database::session&& ts)
        : _db_session(move(s))
        , _token_db_session(move(ts)) {}
    pending_state(pending_state&& ps)
        : _db_session(move(ps._db_session))
        , _token_db_session(move(ps._token_db_session)) {}

    database::session       _db_session;
    token_database::session _token_db_session;

    block_state_ptr _pending_block_state;

    vector<action_receipt> _actions;

    controller::block_status _block_status = controller::block_status::incomplete;

    void
    push() {
        _db_session.push();
        _token_db_session.accept();
    }
};

struct controller_impl {
    controller&             self;
    chainbase::database     db;
    chainbase::database     reversible_blocks; ///< a special database to persist blocks that have successfully been applied but are still reversible
    block_log               blog;
    optional<pending_state> pending;
    block_state_ptr         head;
    fork_database           fork_db;
    token_database          token_db;
    controller::config      conf;
    chain_id_type           chain_id;
    bool                    replaying = false;
    bool                    in_trx_requiring_checks = false; ///< if true, checks that are normally skipped on replay (e.g. auth checks) cannot be skipped
    abi_serializer          system_api;

    /**
    *  Transactions that were undone by pop_block or abort_block, transactions
    *  are removed from this list if they are re-applied in other blocks. Producers
    *  can query this list when scheduling new transactions into blocks.
    */
    map<digest_type, transaction_metadata_ptr> unapplied_transactions;

    void
    pop_block() {
        auto prev = fork_db.get_block(head->header.previous);
        EVT_ASSERT(prev, block_validate_exception, "attempt to pop beyond last irreversible block");

        if(const auto* b = reversible_blocks.find<reversible_block_object,by_num>(head->block_num)) {
            reversible_blocks.remove(*b);
        }

        for(const auto& t : head->trxs) {
            unapplied_transactions[t->signed_id] = t;
        }
        head = prev;
        db.undo();
        token_db.rollback_to_latest_savepoint();
    }

    controller_impl(const controller::config& cfg, controller& s)
        : self(s)
        , db(cfg.state_dir,
             cfg.read_only ? database::read_only : database::read_write,
             cfg.state_size)
        , reversible_blocks(cfg.blocks_dir/config::reversible_blocks_dir_name,
             cfg.read_only ? database::read_only : database::read_write,
             cfg.reversible_cache_size)
        , blog(cfg.blocks_dir)
        , fork_db(cfg.state_dir)
        , token_db(cfg.tokendb_dir)
        , conf(cfg)
        , chain_id(cfg.genesis.compute_chain_id())
        , system_api(contracts::evt_contract_abi()) {

        fork_db.irreversible.connect([&](auto b) {
            on_irreversible(b);
        });
    }

    /**
    *  Plugins / observers listening to signals emited (such as accepted_transaction) might trigger
    *  errors and throw exceptions. Unless those exceptions are caught it could impact consensus and/or
    *  cause a node to fork.
    *
    *  If it is ever desirable to let a signal handler bubble an exception out of this method
    *  a full audit of its uses needs to be undertaken.
    *
    */
    template <typename Signal, typename Arg>
    void
    emit(const Signal& s, Arg&& a) {
        try {
            s(std::forward<Arg>(a));
        }
        catch(boost::interprocess::bad_alloc& e) {
            wlog("bad alloc");
            throw e;
        }
        catch(controller_emit_signal_exception& e) {
            wlog("${details}", ("details", e.to_detail_string()));
            throw e;
        }
        catch(fc::exception& e) {
            wlog("${details}", ("details", e.to_detail_string()));
        }
        catch(...) {
            wlog("signal handler threw exception");
        }
    }

    void
    on_irreversible(const block_state_ptr& s) {
        if(!blog.head())
            blog.read_head();

        const auto& log_head = blog.head();
        EVT_ASSERT(log_head, block_log_exception, "block log head can not be found");
        auto lh_block_num = log_head->block_num();

        db.commit(s->block_num);
        token_db.pop_savepoints(s->block_num);

        if(s->block_num <= lh_block_num) {
            return;
        }

        EVT_ASSERT(s->block_num - 1 == lh_block_num, unlinkable_block_exception, "unlinkable block", ("s->block_num",s->block_num)("lh_block_num", lh_block_num));
        EVT_ASSERT(s->block->previous == log_head->id(), unlinkable_block_exception, "irreversible doesn't link to block log head");
        blog.append(s->block);

        const auto& ubi = reversible_blocks.get_index<reversible_block_index,by_num>();
        auto objitr = ubi.begin();
        while(objitr != ubi.end() && objitr->blocknum <= s->block_num) {
            reversible_blocks.remove( *objitr );
            objitr = ubi.begin();
        }

        emit(self.irreversible_block, s);
    }

    void
    init() {
        /**
      *  The fork database needs an initial block_state to be set before
      *  it can accept any new blocks. This initial block state can be found
      *  in the database (whose head block state should be irreversible) or
      *  it would be the genesis state.
      */
        if(!head) {
            initialize_fork_db();  // set head to genesis state
            initialize_token_db();
            auto end = blog.read_head();
            if(end && end->block_num() > 1) {
                replaying = true;
                ilog( "existing block log, attempting to replay ${n} blocks", ("n",end->block_num()) );

                auto start = fc::time_point::now();
                while(auto next = blog.read_block_by_num(head->block_num + 1)) {
                    self.push_block(next, controller::block_status::irreversible);
                    if(next->block_num() % 100 == 0) {
                        std::cerr << std::setw(10) << next->block_num() << " of " << end->block_num() <<"\r";
                    }
                }

                int rev = 0;
                while(auto obj = reversible_blocks.find<reversible_block_object,by_num>(head->block_num+1)) {
                    ++rev;
                    self.push_block(obj->get_block(), controller::block_status::validated);
                }

                std::cerr<< "\n";
                ilog("${n} reversible blocks replayed", ("n",rev));
                auto end = fc::time_point::now();
                ilog("replayed ${n} blocks in ${duration} seconds, ${mspb} ms/block",
                    ("n", head->block_num)("duration", (end-start).count()/1000000)
                    ("mspb", ((end-start).count()/1000.0)/head->block_num));
                std::cerr<< "\n";
                replaying = false;
            }
            else if(!end) {
                blog.reset_to_genesis(conf.genesis, head->block);
            }
        }

        const auto& ubi = reversible_blocks.get_index<reversible_block_index,by_num>();
        auto objitr = ubi.rbegin();
        if(objitr != ubi.rend()) {
            EVT_ASSERT(objitr->blocknum == head->block_num, fork_database_exception,
                "reversible block database is inconsistent with fork database, replay blockchain",
                ("head",head->block_num)("unconfimed", objitr->blocknum));
        }
        else {
            auto end = blog.read_head();
            EVT_ASSERT(end && end->block_num() == head->block_num, fork_database_exception,
                "fork database exists but reversible block database does not, replay blockchain",
                ("blog_head",end->block_num())("head",head->block_num));
        }

        EVT_ASSERT(db.revision() >= head->block_num, fork_database_exception, "fork database is inconsistent with shared memory",
            ("db",db.revision())("head",head->block_num));

        if(db.revision() > head->block_num) {
            wlog("warning: database revision (${db}) is greater than head block number (${head}), "
               "attempting to undo pending changes",
               ("db",db.revision())("head",head->block_num));
        }
        while(db.revision() > head->block_num) {
            db.undo();
        }
    }

    ~controller_impl() {
        pending.reset();

        db.flush();
        reversible_blocks.flush();
    }

    void
    add_indices() {
        reversible_blocks.add_index<reversible_block_index>(); 

        db.add_index<global_property_multi_index>();
        db.add_index<dynamic_global_property_multi_index>();
        db.add_index<block_summary_multi_index>();
        db.add_index<transaction_multi_index>();
        db.add_index<evt_link_multi_index>();
    }

    /**
    *  Sets fork database head to the genesis state.
    */
    void
    initialize_fork_db() {
        wlog(" Initializing new blockchain with genesis state");
        producer_schedule_type initial_schedule{0, {{N128(evt), conf.genesis.initial_key}}};

        block_header_state genheader;
        genheader.active_schedule       = initial_schedule;
        genheader.pending_schedule      = initial_schedule;
        genheader.pending_schedule_hash = fc::sha256::hash(initial_schedule);
        genheader.header.timestamp      = conf.genesis.initial_timestamp;
        genheader.header.action_mroot   = conf.genesis.compute_chain_id();
        genheader.id                    = genheader.header.id();
        genheader.block_num             = genheader.header.block_num();
        genheader.block_signing_key     = conf.genesis.initial_key;
        
        head        = std::make_shared<block_state>(genheader);
        head->block = std::make_shared<signed_block>(genheader.header);
        fork_db.set(head);
        db.set_revision(head->block_num);

        initialize_database();
    }

    void
    initialize_database() {
        // Initialize block summary index
        for(int i = 0; i < 0x10000; i++) {
            db.create<block_summary_object>([&](block_summary_object&) {});
        }

        const auto& tapos_block_summary = db.get<block_summary_object>(1);
        db.modify(tapos_block_summary, [&](auto& bs) {
            bs.block_id = head->id;
        });

        conf.genesis.initial_configuration.validate();
        db.create<global_property_object>([&](auto& gpo) {
            gpo.configuration = conf.genesis.initial_configuration;
        });
        db.create<dynamic_global_property_object>([](auto&) {});
    }

    void
    initialize_token_db() {
        if(!token_db.exists_domain(".domain")) {
            auto dd = domain_def();
            dd.name = ".domain";
            dd.creator = conf.genesis.initial_key;
            dd.create_time = conf.genesis.initial_timestamp;
            auto r = token_db.add_domain(dd);
            FC_ASSERT(r == 0, "Add `.domain` domain failed");
        }
        if(!token_db.exists_domain(".group")) {
            auto gd = domain_def();
            gd.name = ".group";
            gd.creator = conf.genesis.initial_key;
            gd.create_time = conf.genesis.initial_timestamp;
            auto r = token_db.add_domain(gd);
            FC_ASSERT(r == 0, "Add `.group` domain failed");
        }
        if(!token_db.exists_domain(".suspend")) {
            auto dd = domain_def();
            dd.name = ".suspend";
            dd.creator = conf.genesis.initial_key;
            dd.create_time = conf.genesis.initial_timestamp;
            auto r = token_db.add_domain(dd);
            FC_ASSERT(r == 0, "Add `.suspend` domain failed");
        }
        if(!token_db.exists_domain(".fungible")) {
            auto dd = domain_def();
            dd.name = ".fungible";
            dd.creator = conf.genesis.initial_key;
            dd.create_time = conf.genesis.initial_timestamp;
            auto r = token_db.add_domain(dd);
            FC_ASSERT(r == 0, "Add `.fungible` domain failed");
        }

        initialize_evt_org(token_db, conf.genesis);
    }

    /**
     * @post regardless of the success of commit block there is no active pending block
     */
    void
    commit_block(bool add_to_fork_db) {
        auto reset_pending_on_exit = fc::make_scoped_exit([this] {
            pending.reset();
        });

        try {
            if(add_to_fork_db) {
                pending->_pending_block_state->validated = true;
                auto new_bsp = fork_db.add(pending->_pending_block_state);
                emit(self.accepted_block_header, pending->_pending_block_state);
                head = fork_db.head();
                EVT_ASSERT(new_bsp == head, fork_database_exception, "committed block did not become the new head in fork database");
            }

            if(!replaying) {
                reversible_blocks.create<reversible_block_object>([&](auto& ubo) {
                    ubo.blocknum = pending->_pending_block_state->block_num;
                    ubo.set_block(pending->_pending_block_state->block);
                });
            }

            emit(self.accepted_block, pending->_pending_block_state);
        }
        catch (...) {
            // dont bother resetting pending, instead abort the block
            reset_pending_on_exit.cancel();
            abort_block();
            throw;
        }

        // push the state for pending.
        pending->push();
    }

    // The returned scoped_exit should not exceed the lifetime of the pending which existed when make_block_restore_point was called.
    fc::scoped_exit<std::function<void()>>
    make_block_restore_point() {
        auto orig_block_transactions_size = pending->_pending_block_state->block->transactions.size();
        auto orig_state_transactions_size = pending->_pending_block_state->trxs.size();
        auto orig_state_actions_size      = pending->_actions.size();

        std::function<void()> callback = [this,
                                          orig_block_transactions_size,
                                          orig_state_transactions_size,
                                          orig_state_actions_size]() {
            pending->_pending_block_state->block->transactions.resize(orig_block_transactions_size);
            pending->_pending_block_state->trxs.resize(orig_state_transactions_size);
            pending->_actions.resize(orig_state_actions_size);
        };

        return fc::make_scoped_exit(std::move(callback));
    }

    /**
    *  Adds the transaction receipt to the pending block and returns it.
    */
    template <typename T>
    const transaction_receipt&
    push_receipt(const T& trx, transaction_receipt_header::status_enum status, transaction_receipt_header::type_enum type) {
        pending->_pending_block_state->block->transactions.emplace_back(trx);
        transaction_receipt& r = pending->_pending_block_state->block->transactions.back();
        r.status               = status;
        r.type                 = type;
        return r;
    }

    bool
    failure_is_subjective(const fc::exception& e) {
        auto code = e.code();
        return (code == deadline_exception::code_value);
    }

    void
    check_authorization(const flat_set<public_key_type>& signed_keys, const transaction& trx) {
        auto& conf = db.get<global_property_object>().configuration;

        auto checker = authority_checker(self, signed_keys, token_db, conf.max_authority_depth);
        for(const auto& act : trx.actions) {
            EVT_ASSERT(checker.satisfied(act), unsatisfied_authorization,
                       "${name} action in domain: ${domain} with key: ${key} authorized failed",
                       ("domain", act.domain)("key", act.key)("name", act.name));
        }
    }

    void
    check_authorization(const flat_set<public_key_type>& signed_keys, const action& act) {
        auto& conf = db.get<global_property_object>().configuration;

        auto checker = authority_checker(self, signed_keys, token_db, conf.max_authority_depth);
        EVT_ASSERT(checker.satisfied(act), unsatisfied_authorization,
                   "${name} action in domain: ${domain} with key: ${key} authorized failed",
                   ("domain", act.domain)("key", act.key)("name", act.name));
    }

    transaction_trace_ptr
    push_suspend_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline) {
        try {
            auto reset_in_trx_requiring_checks = fc::make_scoped_exit([old_value=in_trx_requiring_checks,this] {
                in_trx_requiring_checks = old_value;
            });
            in_trx_requiring_checks = true;

            transaction_context trx_context(self, *trx);
            trx_context.deadline = deadline;

            auto trace = trx_context.trace;
            try {
                trx_context.init_for_suspend_trx();
                trx_context.exec();
                trx_context.finalize();

                auto restore = make_block_restore_point();

                trace->receipt = push_receipt(trx->packed_trx,
                                              transaction_receipt::executed,
                                              transaction_receipt::suspend);

                fc::move_append(pending->_actions, move(trx_context.executed));

                emit(self.applied_transaction, trace);

                trx_context.squash();
                restore.cancel();
                return trace;
            }
            catch(const fc::exception& e) {
                trace->except     = e;
                trace->except_ptr = std::current_exception();
                trace->elapsed    = fc::time_point::now() - trx_context.start;
            }
            trx_context.undo_session.undo();

            trace->elapsed = fc::time_point::now() - trx_context.start;

            if(failure_is_subjective(*trace->except)) {
                trace->receipt = push_receipt(trx->packed_trx,
                                              transaction_receipt::soft_fail,
                                              transaction_receipt::suspend);
            }
            else {
                trace->receipt = push_receipt(trx->packed_trx,
                                              transaction_receipt::hard_fail,
                                              transaction_receipt::suspend);
            }
            emit(self.applied_transaction, trace);
            return trace;
        }
        FC_CAPTURE_AND_RETHROW()
    } /// push_scheduled_transaction

    /**
    *  This is the entry point for new transactions to the block state. It will check authorization
    *  and insert a transaction receipt into the pending block.
    */
    transaction_trace_ptr
    push_transaction(const transaction_metadata_ptr& trx,
                     fc::time_point                  deadline,
                     bool                            implicit) {
        EVT_ASSERT(deadline != fc::time_point(), transaction_exception, "deadline cannot be uninitialized");

        transaction_trace_ptr trace;
        try {
            transaction_context trx_context(self, *trx);
            trx_context.deadline = deadline;
            trace                = trx_context.trace;
            try {
                if(implicit) {
                    trx_context.init_for_implicit_trx();
                }
                else {
                    trx_context.init_for_input_trx(trx->trx.signatures.size());
                }

                if(!self.skip_auth_check() && !implicit) {
                    const auto& keys = trx->recover_keys(chain_id);
                    check_authorization(keys, trx->trx);
                }

                trx_context.exec();
                trx_context.finalize();  // Automatically rounds up network and CPU usage in trace and bills payers if successful

                auto restore = make_block_restore_point();

                if(!implicit) {
                    trace->receipt = push_receipt(trx->packed_trx,
                                                  transaction_receipt::executed,
                                                  transaction_receipt::input);
                    pending->_pending_block_state->trxs.emplace_back(trx);
                }
                else {
                    transaction_receipt_header r;
                    r.status       = transaction_receipt::executed;
                    trace->receipt = r;
                }

                fc::move_append(pending->_actions, move(trx_context.executed));

                // call the accept signal but only once for this transaction
                if(!trx->accepted) {
                    emit(self.accepted_transaction, trx);
                    trx->accepted = true;
                }

                emit(self.applied_transaction, trace);

                restore.cancel();
                trx_context.squash();

                if(!implicit) {
                    unapplied_transactions.erase(trx->signed_id);
                }
                return trace;
            }
            catch(const fc::exception& e) {
                trace->except     = e;
                trace->except_ptr = std::current_exception();
            }
            if(!failure_is_subjective(*trace->except)) {
                unapplied_transactions.erase(trx->signed_id);
            }

            return trace;
        }
        FC_CAPTURE_AND_RETHROW((trace))
    }  /// push_transaction

    void
    start_block(block_timestamp_type when, uint16_t confirm_block_count, controller::block_status s) {
        EVT_ASSERT(!pending, block_validate_exception, "pending block is not available");

        EVT_ASSERT(db.revision() == head->block_num, database_exception, "db revision is not on par with head block",
                  ("db.revision()", db.revision())("controller_head_block", head->block_num)("fork_db_head_block", fork_db.head()->block_num));

        auto guard_pending = fc::make_scoped_exit([this]() {
            pending.reset();
        });

        pending = pending_state(db.start_undo_session(true), token_db.new_savepoint_session(db.revision()));

        pending->_block_status = s;

        pending->_pending_block_state                   = std::make_shared<block_state>(*head, when);  // promotes pending schedule (if any) to active
        pending->_pending_block_state->in_current_chain = true;

        pending->_pending_block_state->set_confirmed(confirm_block_count);

        auto was_pending_promoted = pending->_pending_block_state->maybe_promote_pending();

        const auto& gpo = db.get<global_property_object>();
        if(gpo.proposed_schedule_block_num.valid() &&                                                          // if there is a proposed schedule that was proposed in a block ...
           (*gpo.proposed_schedule_block_num <= pending->_pending_block_state->dpos_irreversible_blocknum) &&  // ... that has now become irreversible ...
           pending->_pending_block_state->pending_schedule.producers.size() == 0 &&                            // ... and there is room for a new pending schedule ...
           !was_pending_promoted                                                                               // ... and not just because it was promoted to active at the start of this block, then:
        ) {
            // Promote proposed schedule to pending schedule.
            if(!replaying) {
                ilog("promoting proposed schedule (set in block ${proposed_num}) to pending; current block: ${n} lib: ${lib} schedule: ${schedule} ",
                    ("proposed_num", *gpo.proposed_schedule_block_num)("n", pending->_pending_block_state->block_num)
                    ("lib", pending->_pending_block_state->dpos_irreversible_blocknum)
                    ("schedule", static_cast<producer_schedule_type>(gpo.proposed_schedule)));
            }
            pending->_pending_block_state->set_new_producers(gpo.proposed_schedule);
            db.modify(gpo, [&](auto& gp) {
                gp.proposed_schedule_block_num = optional<block_num_type>();
                gp.proposed_schedule.clear();
            });
        }

        clear_expired_input_transactions();
        guard_pending.cancel();
    }  // start_block

    void
    sign_block(const std::function<signature_type(const digest_type&)>& signer_callback) {
        auto p = pending->_pending_block_state;
        p->sign(signer_callback);

        static_cast<signed_block_header&>(*p->block) = p->header;
    }  /// sign_block

    void
    apply_block(const signed_block_ptr& b, controller::block_status s) {
        try {
            try {
                EVT_ASSERT(b->block_extensions.size() == 0, block_validate_exception, "no supported extensions");
                start_block(b->timestamp, b->confirmed, s);

                auto num_pending_receipts = pending->_pending_block_state->block->transactions.size();
                for(const auto& receipt : b->transactions) {
                    auto trace = transaction_trace_ptr();
                    if(receipt.type == transaction_receipt::input) {
                        auto& pt    = receipt.trx;
                        auto  mtrx  = std::make_shared<transaction_metadata>(pt);
                        
                        trace = push_transaction(mtrx, fc::time_point::maximum(), false);
                    }
                    else if(receipt.type == transaction_receipt::suspend) {
                        // suspend transaction is executed in its parent transaction
                        // so don't execute here
                        num_pending_receipts++;
                        continue;
                    }
                    else {
                        EVT_ASSERT(false, block_validate_exception, "encountered unexpected receipt type");
                    }

                    auto transaction_failed = trace && trace->except;
                    if(transaction_failed) {
                        edump((*trace));
                        throw *trace->except;
                    }
                    EVT_ASSERT(pending->_pending_block_state->block->transactions.size() > 0,
                               block_validate_exception, "expected a receipt",
                               ("block", *b)("expected_receipt", receipt)
                               );
                    EVT_ASSERT(pending->_pending_block_state->block->transactions.size() == num_pending_receipts + 1,
                               block_validate_exception, "expected receipt was not added",
                               ("block", *b)("expected_receipt", receipt)
                               );
                    auto& r = pending->_pending_block_state->block->transactions.back();
                    EVT_ASSERT(r == static_cast<const transaction_receipt_header&>(receipt),
                               block_validate_exception, "receipt does not match",
                              ("producer_receipt", receipt)("validator_receipt", pending->_pending_block_state->block->transactions.back())
                              );

                    num_pending_receipts++;
                }

                finalize_block();

                // this implicitly asserts that all header fields (less the signature) are identical
                EVT_ASSERT(b->id() == pending->_pending_block_state->header.id(),
                       block_validate_exception, "Block ID does not match",
                       ("producer_block_id",b->id())("validator_block_id",pending->_pending_block_state->header.id()));

                // We need to fill out the pending block state's block because that gets serialized in the reversible block log
                // in the future we can optimize this by serializing the original and not the copy

                // we can always trust this signature because,
                //   - prior to apply_block, we call fork_db.add which does a signature check IFF the block is untrusted
                //   - OTHERWISE the block is trusted and therefore we trust that the signature is valid
                // Also, as ::sign_block does not lazily calculate the digest of the block, we can just short-circuit to save cycles
                pending->_pending_block_state->header.producer_signature = b->producer_signature;
                static_cast<signed_block_header&>(*pending->_pending_block_state->block) =  pending->_pending_block_state->header;

                commit_block(false);
                return;
            }
            catch(const fc::exception& e) {
                edump((e.to_detail_string()));
                abort_block();
                throw;
            }
        }
        FC_CAPTURE_AND_RETHROW()
    }  /// apply_block

    void
    push_block(const signed_block_ptr& b, controller::block_status s) {
        //  idump((fc::json::to_pretty_string(*b)));
        EVT_ASSERT(!pending, block_validate_exception, "it is not valid to push a block when there is a pending block");
        try {
            EVT_ASSERT(b, block_validate_exception, "trying to push empty block");
            EVT_ASSERT(s != controller::block_status::incomplete, block_validate_exception, "invalid block status for a completed block");
            emit(self.pre_accepted_block, b);

            bool trust = !conf.force_all_checks && (s == controller::block_status::irreversible || s == controller::block_status::validated);
            auto new_header_state = fork_db.add(b, trust);
            emit(self.accepted_block_header, new_header_state);
            // on replay irreversible is not emitted by fork database, so emit it explicitly here
            if(s == controller::block_status::irreversible) {
                emit(self.irreversible_block, new_header_state);
            }
            maybe_switch_forks(s);
        }
        FC_LOG_AND_RETHROW()
    }

    void
    push_confirmation(const header_confirmation& c) {
        EVT_ASSERT(!pending, block_validate_exception, "it is not valid to push a confirmation when there is a pending block");
        fork_db.add(c);
        emit(self.accepted_confirmation, c);
        maybe_switch_forks();
    }

    void
    maybe_switch_forks(controller::block_status s = controller::block_status::complete) {
        auto new_head = fork_db.head();

        if(new_head->header.previous == head->id) {
            try {
                apply_block(new_head->block, s);
                fork_db.mark_in_current_chain(new_head, true);
                fork_db.set_validity(new_head, true);
                head = new_head;
            }
            catch(const fc::exception& e) {
                fork_db.set_validity(new_head, false);  // Removes new_head from fork_db index, so no need to mark it as not in the current chain.
                throw;
            }
        }
        else if(new_head->id != head->id) {
            ilog("switching forks from ${current_head_id} (block number ${current_head_num}) to ${new_head_id} (block number ${new_head_num})",
                 ("current_head_id", head->id)("current_head_num", head->block_num)("new_head_id", new_head->id)("new_head_num", new_head->block_num));
            auto branches = fork_db.fetch_branch_from(new_head->id, head->id);

            for(auto itr = branches.second.begin(); itr != branches.second.end(); ++itr) {
                fork_db.mark_in_current_chain(*itr, false);
                pop_block();
            }
            EVT_ASSERT(self.head_block_id() == branches.second.back()->header.previous, fork_database_exception,
                      "loss of sync between fork_db and chainbase during fork switch");  // _should_ never fail

            for(auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr) {
                optional<fc::exception> except;
                try {
                    apply_block((*ritr)->block,  (*ritr)->validated ? controller::block_status::validated : controller::block_status::complete);
                    head = *ritr;
                    fork_db.mark_in_current_chain(*ritr, true);
                    (*ritr)->validated = true;
                }
                catch(const fc::exception& e) {
                    except = e;
                }
                if(except) {
                    elog("exception thrown while switching forks ${e}", ("e", except->to_detail_string()));

                    // ritr currently points to the block that threw
                    // if we mark it invalid it will automatically remove all forks built off it.
                    fork_db.set_validity(*ritr, false);

                    // pop all blocks from the bad fork
                    // ritr base is a forward itr to the last block successfully applied
                    auto applied_itr = ritr.base();
                    for( auto itr = applied_itr; itr != branches.first.end(); ++itr ) {
                        fork_db.mark_in_current_chain(*itr, false);
                        pop_block();
                    }
                    EVT_ASSERT(self.head_block_id() == branches.second.back()->header.previous, fork_database_exception,
                              "loss of sync between fork_db and chainbase during fork switch reversal");  // _should_ never fail

                    // re-apply good blocks
                    for(auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr) {
                        apply_block((*ritr)->block, controller::block_status::validated /* we previously validated these blocks*/);
                        head = *ritr;
                        fork_db.mark_in_current_chain(*ritr, true);
                    }
                    throw *except;
                }  // end if exception
            }      /// end for each block in branch
            ilog("successfully switched fork to new head ${new_head_id}", ("new_head_id", new_head->id));
        }
    }  /// push_block

    void
    abort_block() {
        if(pending) {
            for(const auto& t : pending->_pending_block_state->trxs) {
                unapplied_transactions[t->signed_id] = t;
            }
            pending.reset();
        }
    }

    bool
    should_enforce_runtime_limits() const {
        return false;
    }

    void
    set_action_merkle() {
        vector<digest_type> action_digests;
        action_digests.reserve(pending->_actions.size());
        for(const auto& a : pending->_actions) {
            action_digests.emplace_back(a.digest());
        }

        pending->_pending_block_state->header.action_mroot = merkle(move(action_digests));
    }

    void
    set_trx_merkle() {
        vector<digest_type> trx_digests;
        const auto&         trxs = pending->_pending_block_state->block->transactions;
        trx_digests.reserve(trxs.size());
        for(const auto& trx : trxs) {
            trx_digests.emplace_back(trx.digest());
        }

        pending->_pending_block_state->header.transaction_mroot = merkle(move(trx_digests));
    }

    void
    finalize_block() {
        EVT_ASSERT(pending, block_validate_exception, "it is not valid to finalize when there is no pending block");
        try {
            /*
      ilog( "finalize block ${n} (${id}) at ${t} by ${p} (${signing_key}); schedule_version: ${v} lib: ${lib} #dtrxs: ${ndtrxs} ${np}",
            ("n",pending->_pending_block_state->block_num)
            ("id",pending->_pending_block_state->header.id())
            ("t",pending->_pending_block_state->header.timestamp)
            ("p",pending->_pending_block_state->header.producer)
            ("signing_key", pending->_pending_block_state->block_signing_key)
            ("v",pending->_pending_block_state->header.schedule_version)
            ("lib",pending->_pending_block_state->dpos_irreversible_blocknum)
            ("ndtrxs",db.get_index<generated_transaction_multi_index,by_trx_id>().size())
            ("np",pending->_pending_block_state->header.new_producers)
            );
      */
            set_action_merkle();
            set_trx_merkle();

            auto p = pending->_pending_block_state;
            p->id  = p->header.id();

            create_block_summary(p->id);
        }
        FC_CAPTURE_AND_RETHROW()
    }

    void
    create_block_summary(const block_id_type& id) {
        auto block_num = block_header::num_from_id(id);
        auto sid       = block_num & 0xffff;
        db.modify(db.get<block_summary_object, by_id>(sid), [&](block_summary_object& bso) {
            bso.block_id = id;
        });
    }

    void
    clear_expired_input_transactions() {
        //Look for expired transactions in the deduplication list, and remove them.
        auto&       transaction_idx = db.get_mutable_index<transaction_multi_index>();
        const auto& dedupe_index    = transaction_idx.indices().get<by_expiration>();
        auto        now             = self.pending_block_time();
        while((!dedupe_index.empty()) && (now > fc::time_point(dedupe_index.begin()->expiration))) {
            transaction_idx.remove(*dedupe_index.begin());
        }
    }

};  /// controller_impl

controller::controller(const controller::config& cfg)
    : my(new controller_impl(cfg, *this)) {
}

controller::~controller() {
    my->abort_block();
    //close fork_db here, because it can generate "irreversible" signal to this controller,
    //in case if read-mode == IRREVERSIBLE, we will apply latest irreversible block
    //for that we need 'my' to be valid pointer pointing to valid controller_impl.
    my->fork_db.close();
}

void
controller::startup() {
    // ilog( "${c}", ("c",fc::json::to_pretty_string(cfg)) );
    my->add_indices();

    my->head = my->fork_db.head();
    if(!my->head) {
        wlog("No head block in fork db, perhaps we need to replay");
    }
    my->init();
}

chainbase::database&
controller::db() const {
    return my->db;
}

fork_database&
controller::fork_db() const {
    return my->fork_db;
}

token_database&
controller::token_db() const {
    return my->token_db;
}

charge_manager
controller::get_charge_manager() const {
    return charge_manager(*this);
}

void
controller::start_block(block_timestamp_type when, uint16_t confirm_block_count) {
    validate_db_available_size();
    my->start_block(when, confirm_block_count, block_status::incomplete);
}

void
controller::finalize_block() {
    validate_db_available_size();
    my->finalize_block();
}

void
controller::sign_block(const std::function<signature_type(const digest_type&)>& signer_callback) {
    my->sign_block(signer_callback);
}

void
controller::commit_block() {
    validate_db_available_size();
    validate_reversible_available_size();
    my->commit_block(true);
}

void
controller::abort_block() {
    my->abort_block();
}

void
controller::push_block(const signed_block_ptr& b, block_status s) {
    validate_db_available_size();
    validate_reversible_available_size();
    my->push_block(b, s);
}

void
controller::push_confirmation(const header_confirmation& c) {
    validate_db_available_size();
    my->push_confirmation(c);
}

transaction_trace_ptr
controller::push_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline) {
    validate_db_available_size();
    return my->push_transaction(trx, deadline, false);
}

transaction_trace_ptr
controller::push_suspend_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline) {
    validate_db_available_size();
    return my->push_suspend_transaction(trx, deadline);
}

void
controller::check_authorization(const flat_set<public_key_type>& signed_keys, const transaction& trx) {
    return my->check_authorization(signed_keys, trx);
}

void
controller::check_authorization(const flat_set<public_key_type>& signed_keys, const action& act) {
    return my->check_authorization(signed_keys, act);
}

uint32_t
controller::head_block_num() const {
    return my->head->block_num;
}

time_point
controller::head_block_time() const {
    return my->head->header.timestamp;
}

block_id_type
controller::head_block_id() const {
    return my->head->id;
}

account_name
controller::head_block_producer() const {
    return my->head->header.producer;
}

const block_header&
controller::head_block_header() const {
    return my->head->header;
}

block_state_ptr
controller::head_block_state() const {
    return my->head;
}

uint32_t
controller::fork_db_head_block_num() const {
   return my->fork_db.head()->block_num;
}

block_id_type
controller::fork_db_head_block_id() const {
   return my->fork_db.head()->id;
}

time_point
controller::fork_db_head_block_time() const {
   return my->fork_db.head()->header.timestamp;
}

account_name
controller::fork_db_head_block_producer() const {
   return my->fork_db.head()->header.producer;
}

block_state_ptr
controller::pending_block_state() const {
    if(my->pending)
        return my->pending->_pending_block_state;
    return block_state_ptr();
}
time_point
controller::pending_block_time() const {
    EVT_ASSERT(my->pending, block_validate_exception, "no pending block");
    return my->pending->_pending_block_state->header.timestamp;
}

uint32_t
controller::last_irreversible_block_num() const {
    return std::max(my->head->bft_irreversible_blocknum, my->head->dpos_irreversible_blocknum);
}

block_id_type
controller::last_irreversible_block_id() const {
    auto        lib_num             = last_irreversible_block_num();
    const auto& tapos_block_summary = db().get<block_summary_object>((uint16_t)lib_num);

    if(block_header::num_from_id(tapos_block_summary.block_id) == lib_num)
        return tapos_block_summary.block_id;

    return fetch_block_by_number(lib_num)->id();
}

const dynamic_global_property_object&
controller::get_dynamic_global_properties() const {
    return my->db.get<dynamic_global_property_object>();
}

const global_property_object&
controller::get_global_properties() const {
    return my->db.get<global_property_object>();
}

signed_block_ptr
controller::fetch_block_by_id(block_id_type id) const {
    auto state = my->fork_db.get_block(id);
    if(state) {
        return state->block;
    }
    auto bptr = fetch_block_by_number(block_header::num_from_id(id));
    if(bptr && bptr->id() == id) {
        return bptr;
    }
    return signed_block_ptr();
}

signed_block_ptr
controller::fetch_block_by_number(uint32_t block_num) const {
    try {
        auto blk_state = my->fork_db.get_block_in_current_chain_by_num(block_num);
        if(blk_state) {
            return blk_state->block;
        }

        return my->blog.read_block_by_num(block_num);
    }
    FC_CAPTURE_AND_RETHROW((block_num))
}

block_state_ptr
controller::fetch_block_state_by_id(block_id_type id) const {
    auto state = my->fork_db.get_block(id);
    return state;
}

block_state_ptr
controller::fetch_block_state_by_number(uint32_t block_num) const { 
    try {
        auto blk_state = my->fork_db.get_block_in_current_chain_by_num( block_num );
        return blk_state;
    }
    FC_CAPTURE_AND_RETHROW((block_num))
}


block_id_type
controller::get_block_id_for_num(uint32_t block_num) const {
    try {
        auto blk_state = my->fork_db.get_block_in_current_chain_by_num(block_num);
        if(blk_state) {
            return blk_state->id;
        }

        auto signed_blk = my->blog.read_block_by_num(block_num);

        EVT_ASSERT(BOOST_LIKELY(signed_blk != nullptr), unknown_block_exception,
                   "Could not find block: ${block}", ("block", block_num));

        return signed_blk->id();
    }
    FC_CAPTURE_AND_RETHROW((block_num))
}

transaction_id_type
controller::get_trx_id_for_link_id(const link_id_type& link_id) const {
    try {
        if(const auto* l = my->db.find<evt_link_object, by_link_id>(link_id)) {
            return l->trx_id;
        }
        EVT_THROW(evt_link_existed_exception, "EVT-Link is not existed");
    }
    FC_CAPTURE_AND_RETHROW((link_id))
}

void
controller::pop_block() {
    my->pop_block();
}

int64_t
controller::set_proposed_producers(vector<producer_key> producers) {
    const auto& gpo           = get_global_properties();
    auto        cur_block_num = head_block_num() + 1;

    if(gpo.proposed_schedule_block_num.valid()) {
        if(*gpo.proposed_schedule_block_num != cur_block_num)
            return -1;  // there is already a proposed schedule set in a previous block, wait for it to become pending

        if(std::equal(producers.begin(), producers.end(),
                      gpo.proposed_schedule.producers.begin(), gpo.proposed_schedule.producers.end()))
            return -1;  // the proposed producer schedule does not change
    }

    producer_schedule_type sch;

    decltype(sch.producers.cend()) end;
    decltype(end)                  begin;

    if(my->pending->_pending_block_state->pending_schedule.producers.size() == 0) {
        const auto& active_sch = my->pending->_pending_block_state->active_schedule;
        begin                  = active_sch.producers.begin();
        end                    = active_sch.producers.end();
        sch.version            = active_sch.version + 1;
    }
    else {
        const auto& pending_sch = my->pending->_pending_block_state->pending_schedule;
        begin                   = pending_sch.producers.begin();
        end                     = pending_sch.producers.end();
        sch.version             = pending_sch.version + 1;
    }

    if(std::equal(producers.begin(), producers.end(), begin, end))
        return -1;  // the producer schedule would not change

    sch.producers = std::move(producers);

    auto version = sch.version;

    my->db.modify(gpo, [&](auto& gp) {
        gp.proposed_schedule_block_num = cur_block_num;
        gp.proposed_schedule           = std::move(sch);
    });
    return version;
}

void
controller::set_chain_config(const chain_config& config) {
    const auto& gpo = get_global_properties();
    my->db.modify(gpo, [&](auto& gp) {
        gp.configuration = config;
    });
}

const producer_schedule_type&
controller::active_producers() const {
    if(!(my->pending))
        return my->head->active_schedule;
    return my->pending->_pending_block_state->active_schedule;
}

const producer_schedule_type&
controller::pending_producers() const {
    if(!(my->pending))
        return my->head->pending_schedule;
    return my->pending->_pending_block_state->pending_schedule;
}

optional<producer_schedule_type>
controller::proposed_producers() const {
    const auto& gpo = get_global_properties();
    if(!gpo.proposed_schedule_block_num.valid())
        return optional<producer_schedule_type>();

    return gpo.proposed_schedule;
}

bool
controller::skip_auth_check() const {
    return my->replaying && !my->conf.force_all_checks && !my->in_trx_requiring_checks;
}

bool
controller::loadtest_mode() const {
    return my->conf.loadtest_mode;
}

bool
controller::charge_free_mode() const {
    return my->conf.charge_free_mode;
}

bool
controller::contracts_console() const {
    return my->conf.contracts_console;
}

chain_id_type
controller::get_chain_id() const {
    return my->chain_id;
}

const abi_serializer&
controller::get_abi_serializer() const {
    return my->system_api;
}

vector<transaction_metadata_ptr>
controller::get_unapplied_transactions() const {
    vector<transaction_metadata_ptr> result;
    result.reserve(my->unapplied_transactions.size());
    for(const auto& entry : my->unapplied_transactions) {
        result.emplace_back(entry.second);
    }
    return result;
}

void
controller::drop_unapplied_transaction(const transaction_metadata_ptr& trx) {
    my->unapplied_transactions.erase(trx->signed_id);
}

bool
controller::is_producing_block() const {
   if(!my->pending) return false;

   return (my->pending->_block_status == block_status::incomplete);
}

void
controller::validate_expiration(const transaction& trx) const {
    try {
        const auto& chain_configuration = get_global_properties().configuration;

        EVT_ASSERT(time_point(trx.expiration) >= pending_block_time(),
                   expired_tx_exception,
                   "transaction has expired, "
                   "expiration is ${trx.expiration} and pending block time is ${pending_block_time}",
                   ("trx.expiration", trx.expiration)("pending_block_time", pending_block_time()));
        EVT_ASSERT(time_point(trx.expiration) <= pending_block_time() + fc::seconds(chain_configuration.max_transaction_lifetime),
                   tx_exp_too_far_exception,
                   "Transaction expiration is too far in the future relative to the reference time of ${reference_time}, "
                   "expiration is ${trx.expiration} and the maximum transaction lifetime is ${max_til_exp} seconds",
                   ("trx.expiration", trx.expiration)("reference_time", pending_block_time())("max_til_exp", chain_configuration.max_transaction_lifetime));
    }
    FC_CAPTURE_AND_RETHROW((trx))
}

void
controller::validate_tapos(const transaction& trx) const {
    try {
        const auto& tapos_block_summary = db().get<block_summary_object>((uint16_t)trx.ref_block_num);

        //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
        EVT_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
                   "Transaction's reference block did not match. Is this transaction from a different fork?",
                   ("tapos_summary", tapos_block_summary));
    }
    FC_CAPTURE_AND_RETHROW()
}

void
controller::validate_db_available_size() const {
   const auto free = db().get_segment_manager()->get_free_memory();
   const auto guard = my->conf.state_guard_size;
   EVT_ASSERT(free >= guard, database_guard_exception, "database free: ${f}, guard size: ${g}", ("f", free)("g",guard));
}

void
controller::validate_reversible_available_size() const {
   const auto free = my->reversible_blocks.get_segment_manager()->get_free_memory();
   const auto guard = my->conf.reversible_guard_size;
   EVT_ASSERT(free >= guard, reversible_guard_exception, "reversible free: ${f}, guard size: ${g}", ("f", free)("g",guard));
}

bool
controller::is_known_unexpired_transaction(const transaction_id_type& id) const {
    return db().find<transaction_object, by_trx_id>(id);
}

flat_set<public_key_type>
controller::get_required_keys(const transaction& trx, const flat_set<public_key_type>& candidate_keys) const {
    const static uint32_t max_authority_depth = my->conf.genesis.initial_configuration.max_authority_depth;
    auto checker = authority_checker(*this, candidate_keys, my->token_db, max_authority_depth);

    for(const auto& act : trx.actions) {
        EVT_ASSERT(checker.satisfied(act), unsatisfied_authorization,
                   "${name} action in domain: ${domain} with key: ${key} authorized failed",
                   ("domain", act.domain)("key", act.key)("name", act.name));
    }

    auto keys = checker.used_keys();
    if(trx.payer.type() == address::public_key_t) {
        keys.emplace(trx.payer.get_public_key());
    }
    return keys;
}

flat_set<public_key_type>
controller::get_suspend_required_keys(const transaction& trx, const flat_set<public_key_type>& candidate_keys) const {
    const static uint32_t max_authority_depth = my->conf.genesis.initial_configuration.max_authority_depth;
    auto checker = authority_checker(*this, candidate_keys, my->token_db, max_authority_depth);

    for(const auto& act : trx.actions) {
        checker.satisfied(act);
    }

    auto keys = checker.used_keys();
    if(trx.payer.type() == address::public_key_t) {
        keys.emplace(trx.payer.get_public_key());
    }
    return keys;
}

flat_set<public_key_type>
controller::get_suspend_required_keys(const proposal_name& name, const flat_set<public_key_type>& candidate_keys) const {
    suspend_def suspend;
    my->token_db.read_suspend(name, suspend);
    
    return get_suspend_required_keys(suspend.trx, candidate_keys);
}

uint32_t
controller::get_charge(const transaction& trx, size_t signautres_num) const {   
    auto packed_trx = packed_transaction(trx);
    auto charge     = get_charge_manager();
    return charge.calculate(packed_trx, signautres_num);
}

}}  // namespace evt::chain
