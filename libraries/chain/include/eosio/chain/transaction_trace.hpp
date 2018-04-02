/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

#include <numeric>

namespace eosio { namespace chain {

   struct action_trace {
      account_name               receiver;
      action                     act;
      string                     console;
      uint32_t                   region_id;
      uint32_t                   cycle_index;

      fc::microseconds           _profiling_us = fc::microseconds(0);
   };

   struct transaction_trace : transaction_receipt {
      using transaction_receipt::transaction_receipt;

      vector<action_trace>       action_traces;

      fc::microseconds           _profiling_us = fc::microseconds(0);
      fc::microseconds           _setup_profiling_us = fc::microseconds(0);
   };
} } // eosio::chain

FC_REFLECT( eosio::chain::action_trace, (receiver)(act)(console)(region_id)(cycle_index)(_profiling_us) )
FC_REFLECT_ENUM( eosio::chain::transaction_receipt::status_enum, (executed)(soft_fail)(hard_fail) )
FC_REFLECT_DERIVED( eosio::chain::transaction_trace, (eosio::chain::transaction_receipt), (action_traces)(_profiling_us)(_setup_profiling_us) )
