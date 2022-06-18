/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
*/
#pragma once
#include <chainbase/chainbase.hpp>
#include <jmzk/chain/config.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

struct validator_context {
    account_name name;
};

class staking_context {
public:
    uint32_t period_version   = 0;  ///< sequentially incrementing version number
    uint32_t period_start_num = 0;
};

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::staking_context, (period_version)(period_start_num));
