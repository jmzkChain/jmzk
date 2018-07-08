/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/chain_config.hpp>
#include <evt/chain/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace evt { namespace chain {

struct genesis_state {
    genesis_state();

    static const string evt_root_key;

    chain_config initial_configuration = {
        .max_block_net_usage            = config::default_max_block_net_usage,
        .target_block_net_usage_pct     = config::default_target_block_net_usage_pct,
        .max_transaction_net_usage      = config::default_max_transaction_net_usage,
        .base_per_transaction_net_usage = config::default_base_per_transaction_net_usage,

        .max_transaction_lifetime       = config::default_max_trx_lifetime,
        .max_authority_depth            = config::default_max_auth_depth};

    time_point      initial_timestamp;
    public_key_type initial_key;

    /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
    chain_id_type compute_chain_id() const;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::genesis_state,
           (initial_timestamp)(initial_key)(initial_configuration))
