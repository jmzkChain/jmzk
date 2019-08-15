/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

struct chain_stake_config {
    int32_t unstake_pending_days = config::default_unstake_pending_days;
    int32_t cycles_per_period    = config::default_cycles_per_period;
    int32_t blocks_per_phase     = config::default_blocks_per_phase;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::chain_stake_config, (unstake_pending_days)(cycles_per_period)(blocks_per_phase));
