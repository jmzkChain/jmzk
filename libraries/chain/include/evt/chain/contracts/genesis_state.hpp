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
      .target_block_size              = config::default_target_block_size,
      .max_block_size                 = config::default_max_block_size,
      .target_block_acts_per_scope    = config::default_target_block_acts_per_scope,
      .max_block_acts_per_scope       = config::default_max_block_acts_per_scope,
      .target_block_acts              = config::default_target_block_acts,
      .max_block_acts                 = config::default_max_block_acts,
      .real_threads                   = 0, // TODO: unused?
      .max_storage_size               = config::default_max_storage_size,
      .max_transaction_lifetime       = config::default_max_trx_lifetime,
      .max_authority_depth            = config::default_max_auth_depth,
      .max_transaction_exec_time      = 0, // TODO: unused?
      .max_inline_depth               = config::default_max_inline_depth,
      .max_inline_action_size         = config::default_max_inline_action_size,
      .max_generated_transaction_size = config::default_max_gen_trx_size
   };

   time_point                               initial_timestamp;
   public_key_type                          initial_key;

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
