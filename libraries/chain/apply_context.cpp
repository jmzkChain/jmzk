/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <algorithm>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/execution/execution_context.hpp>

namespace evt { namespace chain {

static inline void
print_debug(const action_trace& ar) {
   if(!ar.console.empty()) {
      auto prefix = fc::format_string(
                                      "\n[${n}, ${d}-${k}]",
                                      fc::mutable_variant_object()
                                      ("n", ar.act.name)
                                      ("d", ar.act.domain)
                                      ("k", ar.act.key));
      dlog(prefix + ": CONSOLE OUTPUT BEGIN =====================\n"
           + ar.console
           + prefix + ": CONSOLE OUTPUT END   =====================" );
   }
}

void
apply_context::exec_one(action_trace& trace) {
    using namespace contracts;

    auto start = std::chrono::steady_clock::now();

    auto r            = action_receipt();
    r.act_digest      = digest_type::hash(act);
    r.global_sequence = next_global_sequence();

    trace.trx_id            = trx_context.trx.id;
    trace.block_num         = control.pending_block_state()->block_num;
    trace.block_time        = control.pending_block_time();
    trace.producer_block_id = control.pending_producer_block_id();
    trace.act               = act;

    try {
        try {
            if(act.index_ == -1) {
                act.index_ = control.execution_context().index_of(act.name);
            }
            control.execution_context().invoke<apply_action, void>(act.index_, *this);
        }
        FC_RETHROW_EXCEPTIONS(warn, "pending console output: ${console}", ("console", fmt::to_string(_pending_console_output)));
    }
    catch(fc::exception& e) {
        trace.receipt = r; // fill with known data
        trace.except  = e;
        finalize_trace(trace, start);
        throw;
    }

    trace.receipt = r;

    trx_context.executed.emplace_back(move(r));

    finalize_trace(trace, start);

    if(control.contracts_console()) {
        print_debug(trace);
    }
}

void
apply_context::finalize_trace(action_trace& trace, const std::chrono::steady_clock::time_point& start) {
    trace.console = fmt::to_string(_pending_console_output);
    reset_console();
    trace.elapsed = fc::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count());
}

void
apply_context::exec(action_trace& trace) {
    exec_one(trace);
}

bool
apply_context::has_authorized(const domain_name& domain, const domain_key& key) const {
    return act.domain == domain && act.key == key;
}

void
apply_context::reset_console() {
    _pending_console_output = fmt::memory_buffer();
}

uint64_t
apply_context::next_global_sequence() {
    const auto& p = control.get_dynamic_global_properties();
    db.modify(p, [&](auto& dgp) {
        ++dgp.global_action_sequence;
    });
    return p.global_action_sequence;
}

}}  // namespace evt::chain
