/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/transaction_object.hpp>

namespace evt { namespace chain {

transaction_context::transaction_context(controller&           c,
                                         transaction_metadata& t,
                                         fc::time_point        s)
    : control(c)
    , undo_session(c.token_db().new_savepoint_session())
    , trx(t)
    , trace(std::make_shared<transaction_trace>())
    , start(s) {
    trace->id = trx.id;
    executed.reserve(trx.total_actions());
    FC_ASSERT(trx.trx.transaction_extensions.size() == 0, "we don't support any extensions yet");
}

void
transaction_context::init() {
    FC_ASSERT(!is_initialized, "cannot initialize twice");
    checktime();  // Fail early if deadline has already been exceeded
    is_initialized = true;
}

void
transaction_context::init_for_implicit_trx() {
    init();
}

void
transaction_context::init_for_input_trx(uint32_t num_signatures) {
    auto& t = trx.trx;
    is_input  = true;
    control.validate_expiration(t);
    control.validate_tapos(t);
    init();
    record_transaction(trx.id, t.expiration);  /// checks for dupes
}

void
transaction_context::init_for_deferred_trx() {
    trace->is_delay = true;
    init();
}

void
transaction_context::exec() {
    FC_ASSERT(is_initialized, "must first initialize");

    for(const auto& act : trx.trx.actions) {
        trace->action_traces.emplace_back();
        dispatch_action(trace->action_traces.back(), act);
    }
}

void
transaction_context::finalize() {
    FC_ASSERT(is_initialized, "must first initialize");
    trace->elapsed = fc::time_point::now() - start;
}

void
transaction_context::squash() {
    undo_session.squash();
}

void
transaction_context::checktime() const {
    auto now = fc::time_point::now();
    if(BOOST_UNLIKELY(now > deadline)) {
        EVT_THROW(deadline_exception, "deadline exceeded", ("now", now)("deadline", deadline)("start", start));
    }
}

void
transaction_context::dispatch_action(action_trace& trace, const action& a) {
    apply_context acontext(control, *this, a);

    try {
        acontext.exec();
    }
    catch(...) {
        trace = move(acontext.trace);
        throw;
    }

    trace = move(acontext.trace);
}

void
transaction_context::record_transaction(const transaction_id_type& id, fc::time_point_sec expire) {
    try {
        control.db().create<transaction_object>([&](transaction_object& transaction) {
            transaction.trx_id     = id;
            transaction.expiration = expire;
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
