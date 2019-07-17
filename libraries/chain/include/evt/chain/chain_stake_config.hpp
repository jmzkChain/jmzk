/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

struct chain_stake_config {
    int32_t demand_R = config::default_demand_R;
    int32_t demand_T = config::default_demand_T;
    int32_t demand_Q = config::default_demand_Q;
    int32_t demand_W = config::default_demand_W;

    int32_t fixed_R = config::default_fixed_R;
    int32_t fixed_T = config::default_fixed_T;

    int32_t unstake_pending_days = config::default_unstake_pending_days;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::chain_stake_config,
    (demand_R)(demand_T)(demand_Q)(demand_W)(fixed_R)(fixed_T)(unstake_pending_days));
