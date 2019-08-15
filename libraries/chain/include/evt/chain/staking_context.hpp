/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <chainbase/chainbase.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

struct validator_context {
    account_name name;
};

class staking_context {
public:
    uint32_t period_version   = 0;  ///< sequentially incrementing version number
    uint32_t period_start_num = 0;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::staking_context, (period_version)(period_start_num));
