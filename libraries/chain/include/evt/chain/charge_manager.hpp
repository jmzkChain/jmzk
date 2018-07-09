/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/action.hpp>

namespace evt { namespace chain {

template<typename T>
struct get_charge {
    size_t
    operator(const action& act) {
        FC_ASSERT(act.name == T::get_name());
    }
};


}}  // namespace evt::chain
