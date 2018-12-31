/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <utility>
#include <type_traits>
#include <evt/chain/contracts/types.hpp>

namespace evt { namespace chain { namespace contracts {

#define case_act(name)                                                 \
    case N(name): {                                                    \
        static_assert(name::get_action_name().value == N(name));       \
        if constexpr (std::is_void<RType>::value) {                    \
            Invoker<name>::invoke(std::forward<Args>(args)...);        \
        }                                                              \
        else {                                                         \
            return Invoker<name>::invoke(std::forward<Args>(args)...); \
        }                                                              \
        break;                                                         \
    }

template <typename RType, template<typename> typename Invoker>
struct types_invoker {

template<typename ... Args>
static RType
invoke(name n, Args&&... args) {
    switch(n.value) {
    case_act(newdomain)
    case_act(issuetoken)
    case_act(transfer)
    case_act(destroytoken)
    case_act(newgroup)
    case_act(updategroup)
    case_act(updatedomain)
    case_act(newfungible)
    case_act(updfungible)
    case_act(issuefungible)
    case_act(transferft)
    case_act(recycleft)
    case_act(destroyft)
    case_act(evt2pevt)
    case_act(addmeta)
    case_act(newsuspend)
    case_act(cancelsuspend)
    case_act(aprvsuspend)
    case_act(execsuspend)
    case_act(paycharge)
    case_act(everipass)
    case_act(everipay)
    case_act(prodvote)
    case_act(updsched)
    case_act(newlock)
    case_act(aprvlock)
    case_act(tryunlock)
    default: {
        EVT_THROW(action_type_exception, "Unknown action name: ${name}", ("name",n));
    }
    }  // switch
}

};

#undef case_act

}}}  // namespace evt::chain::contracts
