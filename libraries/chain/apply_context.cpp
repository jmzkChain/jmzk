/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/apply_context.hpp>

#include <algorithm>
#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/execution_context_impl.hpp>
#include <jmzk/chain/transaction_context.hpp>
#include <jmzk/chain/global_property_object.hpp>
#include <jmzk/chain/contracts/jmzk_contract.hpp>

namespace jmzk { namespace chain {

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

    trace.trx_id            = trx_context.trx_meta->id;
    trace.block_num         = control.pending_block_state()->block_num;
    trace.block_time        = control.pending_block_time();
    trace.producer_block_id = control.pending_producer_block_id();
    trace.act               = act;

    try {
        try {
            if(act.index_ == -1) {
                act.index_ = exec_ctx.index_of(act.name);
            }
            if(act.index_ == exec_ctx.index_of<contracts::paybonus>()) {
                goto next;
            }
            exec_ctx.invoke<apply_action, void>(act.index_, *this);
        }
        FC_RETHROW_EXCEPTIONS(warn, "pending console output: ${console}", ("console", fmt::to_string(_pending_console_output)));
    }
    catch(fc::exception& e) {
        trace.receipt = r; // fill with known data
        trace.except  = e;
        finalize_trace(trace, start);
        throw;
    }

next:
    trace.receipt = r;
    trx_context.executed.emplace_back(move(r));

    finalize_trace(trace, start);

    if(control.contracts_console()) {
        print_debug(trace);
    }
}

void
apply_context::finalize_trace(action_trace& trace, const std::chrono::steady_clock::time_point& start) {
    using namespace std::chrono;

    trace.console = fmt::to_string(_pending_console_output);
    trace.elapsed = fc::microseconds(duration_cast<microseconds>(steady_clock::now() - start).count());
    
    trace.generated_actions = std::move(_generated_actions);
    trace.new_ft_holders    = std::move(_new_ft_holders);
}

void
apply_context::exec(action_trace& trace) {
    exec_one(trace);
}

bool
apply_context::has_authorized(const domain_name& domain, const domain_key& key) const {
    return act.domain == domain && act.key == key;
}

uint64_t
apply_context::next_global_sequence() {
    const auto& p = control.get_dynamic_global_properties();
    db.modify(p, [&](auto& dgp) {
        ++dgp.global_action_sequence;
    });
    return p.global_action_sequence;
}

}}  // namespace jmzk::chain
