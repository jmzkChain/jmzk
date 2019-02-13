/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/chain_config.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace evt { namespace chain {

using contracts::group_def;
using contracts::fungible_def;

struct genesis_state {
    genesis_state();

    static const string evt_root_key;

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

        .evt_link_expired_secs          = config::default_evt_link_expired_secs
    };

    time_point      initial_timestamp;
    public_key_type initial_key;

    group_def       evt_org;
    fungible_def    evt;
    fungible_def    pevt;

    /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
    chain_id_type compute_chain_id() const;

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

}}  // namespace evt::chain

FC_REFLECT(evt::chain::genesis_state,
           (initial_timestamp)(initial_key)(evt_org)(evt)(pevt)(initial_configuration));
