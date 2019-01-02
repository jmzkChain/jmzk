/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/charge_manager.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/transaction_object.hpp>

namespace evt { namespace chain {

transaction_context::transaction_context(controller&           c,
                                         transaction_metadata& t,
                                         fc::time_point        s)
    : control(c)
    , undo_session()
    , undo_token_session()
    , trx(t)
    , trace(std::make_shared<transaction_trace>())
    , start(s)
    , net_usage(trace->net_usage) {

    if(!c.skip_db_sessions()) {
        undo_session       = c.db().start_undo_session(true);
        undo_token_session = c.token_db().new_savepoint_session();
    }
    trace->id = trx.id;
    executed.reserve(trx.total_actions() + 1); // one for paycharge action

    if(!trx.trx.transaction_extensions.empty()) {
        for(auto& ext : trx.trx.transaction_extensions) {
            FC_ASSERT(std::get<0>(ext) <= (uint16_t)transaction_ext::max_value, "we don't support this extensions yet");            
        }
    }
}

void
transaction_context::init(uint64_t initial_net_usage) {
    EVT_ASSERT(!is_initialized, transaction_exception, "cannot initialize twice");
    EVT_ASSERT(!trx.trx.actions.empty(), tx_no_action, "There's any actions in this transaction");
    
    check_time();    // Fail early if deadline has already been exceeded
    if(!control.charge_free_mode()) {
        check_charge();  // Fail early if max charge has already been exceeded
        check_paid();    // Fail early if there's no remaining available EVT & Pinned EVT tokens
    }

    auto& config = control.get_global_properties().configuration;
    net_limit    = config.max_transaction_net_usage;

    if(initial_net_usage > 0) {
        add_net_usage(initial_net_usage);  // Fail early if current net usage is already greater than the calculated limit
    }

    is_initialized = true;
}

void
transaction_context::init_for_implicit_trx() {
    init(0);
}

void
transaction_context::init_for_input_trx(bool skip_recording) {
    auto& t  = trx.trx;
    is_input = true;
    if(!control.loadtest_mode() || !control.skip_trx_checks()) {
        control.validate_expiration(t);
        control.validate_tapos(t);
    }

    auto& ptrx = trx.packed_trx;
    init(ptrx.get_unprunable_size() + ptrx.get_prunable_size());

    if(!skip_recording) {
        record_transaction(trx.id, t.expiration);  /// checks for dupes
    }
}

void
transaction_context::init_for_suspend_trx() {
    trace->is_suspend = true;
    init(0);
}

void
transaction_context::exec() {
    EVT_ASSERT(is_initialized, transaction_exception, "must first initialize");

    for(const auto& act : trx.trx.actions) {
        trace->action_traces.emplace_back();
        dispatch_action(trace->action_traces.back(), act);
    }
}

void
transaction_context::finalize() {
    EVT_ASSERT(is_initialized, transaction_exception, "must first initialize");

    if(charge) {
        // in charge-free mode, charge always be zero
        finalize_pay();
    }

    trace->charge  = charge;
    trace->elapsed = fc::time_point::now() - start;
}

void
transaction_context::squash() {
    if(undo_session) {
        undo_session->squash();
    }
    if(undo_token_session) {
        undo_token_session->squash();
    }
}

void transaction_context::undo() {
    if(undo_session) {
        undo_session->undo();
    }
    if(undo_token_session) {
        undo_token_session->undo();
    }
}

void
transaction_context::check_time() const {
    auto now = fc::time_point::now();
    if(BOOST_UNLIKELY(now > deadline)) {
        EVT_THROW(deadline_exception, "deadline exceeded", ("now", now)("deadline", deadline)("start", start));
    }
}

void
transaction_context::check_charge() {
    auto cm = control.get_charge_manager();
    charge = cm.calculate(trx.packed_trx);
    if(charge > trx.trx.max_charge) {
        EVT_THROW(max_charge_exceeded_exception, "max charge exceeded, expected: ${ex}, max provided: ${mp}",
            ("ex",charge)("mp",trx.trx.max_charge));
    }
}

void
transaction_context::check_paid() const {
    auto& tokendb = control.token_db();
    auto& payer = trx.trx.payer;

    switch(payer.type()) {
    case address::reserved_t: {
        EVT_THROW(payer_exception, "Reserved address cannot be used to be payer");
    }
    case address::public_key_t: {
        if(!is_input) {
            // if it's not input transaction (suspend transaction)
            // no need to check signature here, have checked in contract
            break;
        }
        auto& keys = trx.recover_keys(control.get_chain_id());
        if(keys.find(payer.get_public_key()) == keys.end()) {
            EVT_THROW(payer_exception, "Payer: ${p} needs to sign this transaction.", ("p",payer));
        }
        break;
    }
    case address::generated_t: {
        auto  prefix = payer.get_prefix();
        auto& key    = payer.get_key();

        switch(prefix.value) {
        case N(.domain): {
            for(auto& act : trx.trx.actions) {
                EVT_ASSERT(act.domain == key, payer_exception, "Only actions in '${d}' domain can be paid by the payer", ("d",act.domain));
            }
            break;
        }
        case N(.fungible): {
            EVT_ASSERT(key != N128(EVT_SYM_ID) && key != N128(PEVT_SYM_ID), payer_exception, "EVT or PEVT is not allowed to use this payer");
            for(auto& act : trx.trx.actions) {
                EVT_ASSERT(act.domain == N128(.fungible) && act.key == key, payer_exception, "Only actions with S#${d} fungible can be paid by the payer", ("d",act.key));
            }
            break;
        }
        default: {
            EVT_THROW(payer_exception, "Only domain or fungible generated address can be payer");
            break;
        }
        }  // switch
        break;
    }
    }  // switch
    
    asset evt, pevt;
    tokendb.read_asset_no_throw(payer, pevt_sym(), pevt);
    if(pevt.amount() > charge) {
        return;
    }
    tokendb.read_asset_no_throw(payer, evt_sym(), evt);
    if(pevt.amount() + evt.amount() >= charge) {
        return;
    }
    EVT_THROW(charge_exceeded_exception, "There are only ${e} and ${p} left, but charge is ${c}", ("e",evt)("p",pevt)("c",charge));
}

void
transaction_context::finalize_pay() {
    auto pcact = paycharge();

    pcact.payer  = trx.trx.payer;
    pcact.charge = charge;

    auto act   = action();
    act.name   = paycharge::get_name();
    act.data   = fc::raw::pack(pcact);
    act.domain = N128(.charge);
    
    switch(pcact.payer.type()) {
    case address::public_key_t: {
        act.key = N128(.public-key);
        break;
    }
    case address::generated_t: {
        act.key = N128(.generated);
        break;
    }
    }  // switch

    trace->action_traces.emplace_back();
    dispatch_action(trace->action_traces.back(), act);
}

void
transaction_context::check_net_usage() const {
    if(!control.skip_trx_checks()) {
        EVT_ASSERT(net_usage < net_limit, tx_net_usage_exceeded, "transaction net usage is too high: ${net_usage} > ${net_limit}", 
            ("net_usage", net_usage)("net_limit", net_limit));
    }
}

void
transaction_context::dispatch_action(action_trace& trace, const action& act) {
    apply_context apply(control, *this, act);

    apply.exec(trace);
}

void
transaction_context::record_transaction(const transaction_id_type& id, fc::time_point_sec expire) {
    try {
        control.db().create<transaction_object>([&](transaction_object& transaction) {
            transaction.trx_id     = id;
            transaction.expiration = expire;
            transaction.block_num  = control.pending_block_state()->block_num;
        });
    }
    catch(const boost::interprocess::bad_alloc&) {
        throw;
    }
    catch(...) {
        EVT_ASSERT(false, tx_duplicate,
                   "duplicate transaction ${id}", ("id", id));
    }
}  /// record_transaction

}}  // namespace evt::chain
