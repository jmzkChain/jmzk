/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <algorithm>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/types_invoker.hpp>
#include <evt/chain/contracts/evt_contract.hpp>

namespace evt { namespace chain {

static inline void
print_debug(const action_trace& ar) {
   if (!ar.console.empty()) {
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

action_trace
apply_context::exec_one() {
    using namespace contracts;

    auto start = fc::time_point::now();
    try {
        types_invoker<void, apply_action>::invoke(act.name, *this);
    }
    FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

    action_receipt r;
    r.act_digest      = digest_type::hash(act);
    r.global_sequence = next_global_sequence();

    auto t    = action_trace(r);
    t.trx_id  = trx_context.trx.id;
    t.act     = act;
    t.console = _pending_console_output.str();

    trx_context.executed.emplace_back(std::move(r));
    
    print_debug(t);
    reset_console();

    t.elapsed = fc::time_point::now() - start;
    return t;
}

void
apply_context::exec() {
    trace = exec_one();
}  /// exec()

bool
apply_context::has_authorized(const domain_name& domain, const domain_key& key) const {
    return act.domain == domain && act.key == key;
}

void
apply_context::reset_console() {
    _pending_console_output = std::ostringstream();
    _pending_console_output.setf(std::ios::scientific, std::ios::floatfield);
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
