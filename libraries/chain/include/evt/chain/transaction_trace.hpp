/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/block.hpp>

#include <numeric>

namespace evt { namespace chain {

   struct action_trace {
      action                     act;
      string                     console;

      fc::microseconds           _profiling_us = fc::microseconds(0);
   };

   struct transaction_trace : transaction_receipt {
      using transaction_receipt::transaction_receipt;

      vector<action_trace>       action_traces;

      optional<digest_type>      packed_trx_digest;
      uint64_t                   region_id;
      uint64_t                   cycle_index;
      uint64_t                   shard_index;

      fc::microseconds           _profiling_us = fc::microseconds(0);
      fc::microseconds           _setup_profiling_us = fc::microseconds(0);
   };
} } // evt::chain

FC_REFLECT( evt::chain::action_trace, (act)(console)(_profiling_us) )
FC_REFLECT_ENUM( evt::chain::transaction_receipt::status_enum, (executed)(soft_fail)(hard_fail) )
FC_REFLECT_DERIVED( evt::chain::transaction_trace, (evt::chain::transaction_receipt), (action_traces)(packed_trx_digest)(region_id)(cycle_index)(shard_index)(_profiling_us)(_setup_profiling_us) )
