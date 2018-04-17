/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/chain_config.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace evt { namespace chain { namespace contracts {

struct genesis_state_type {
   chain_config   initial_configuration = {
      .per_signature_cpu_usage        = config::default_per_signature_cpu_usage,
      .max_transaction_cpu_usage      = config::default_max_transaction_cpu_usage,
      .max_transaction_net_usage      = config::default_max_transaction_net_usage,
      .max_block_cpu_usage            = config::default_max_block_cpu_usage,
      .target_block_cpu_usage_pct     = config::default_target_block_cpu_usage_pct,
      .max_block_net_usage            = config::default_max_block_net_usage,
      .target_block_net_usage_pct     = config::default_target_block_net_usage_pct,
      .max_transaction_lifetime       = config::default_max_trx_lifetime,
      .max_transaction_exec_time      = 0, // TODO: unused?
      .max_authority_depth            = config::default_max_auth_depth,
      .max_inline_depth               = config::default_max_inline_depth,
      .max_inline_action_size         = config::default_max_inline_action_size,
      .max_generated_transaction_count = config::default_max_gen_trx_count
   };

   time_point                               initial_timestamp = fc::time_point::from_iso_string( "2018-03-01T12:00:00" );;
   public_key_type                          initial_key = fc::variant("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV").as<public_key_type>();

   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;
};

} } } // namespace evt::contracts


FC_REFLECT(evt::chain::contracts::genesis_state_type,
           (initial_timestamp)(initial_key)(initial_configuration)(initial_chain_id))
