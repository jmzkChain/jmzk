/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/chain/chain_config.hpp>
#include <jmzk/chain/types.hpp>
#include <jmzk/chain/contracts/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace jmzk { namespace chain {

using contracts::group_def;
using contracts::fungible_def_genesis;
using contracts::fungible_def;

struct genesis_state {
    genesis_state();

    static const string jmzk_root_key;

    chain_config initial_configuration = {
        .max_block_net_usage            = config::default_max_block_net_usage,
        .target_block_net_usage_pct     = config::default_target_block_net_usage_pct,
        .max_transaction_net_usage      = config::default_max_transaction_net_usage,
        .base_per_transaction_net_usage = config::default_base_per_transaction_net_usage,

        .base_network_charge_factor     = config::default_base_network_charge_factor,
        .base_storage_charge_factor     = config::default_base_storage_charge_factor,
        .base_cpu_charge_factor         = config::default_base_cpu_charge_factor,
        .global_charge_factor           = config::default_global_charge_factor,

        .max_transaction_lifetime       = config::default_max_trx_lifetime,
        .max_authority_depth            = config::default_max_auth_depth,

        .jmzk_link_expired_secs          = config::default_jmzk_link_expired_secs
    };

    time_point      initial_timestamp;
    public_key_type initial_key;

    group_def            jmzk_org;
    fungible_def_genesis jmzk;
    fungible_def_genesis pjmzk;

    /**
     * Get the chain_id corresponding to this genesis state.
     *
     * This is the SHA256 serialization of the genesis_state.
     */
    chain_id_type compute_chain_id() const;

    // Get jmzk & Pjmzk FTs
    fungible_def get_jmzk_ft() const;
    fungible_def get_pjmzk_ft() const;

    friend inline bool
    operator==(const genesis_state& lhs, const genesis_state& rhs) {
        return std::tie(lhs.initial_configuration, lhs.initial_timestamp, lhs.initial_key) 
            == std::tie(lhs.initial_configuration, rhs.initial_timestamp, rhs.initial_key);
    };

    friend inline bool
    operator!=(const genesis_state& lhs, const genesis_state& rhs) {
        return !(lhs == rhs);
    }
};

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::genesis_state,
           (initial_timestamp)(initial_key)(jmzk_org)(jmzk)(pjmzk)(initial_configuration));
