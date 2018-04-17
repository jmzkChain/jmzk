/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/types.hpp>
#include <evt/chain/config.hpp>

namespace evt { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config {
   uint32_t   per_signature_cpu_usage;        ///< the cpu usage billed for every signature on a transaction

   uint32_t   max_transaction_cpu_usage;      ///< the maximum objectively measured cpu usage that the chain will allow regardless of account limits
   uint32_t   max_transaction_net_usage;      ///< the maximum objectively measured net usage that the chain will allow regardless of account limits

   uint64_t   max_block_cpu_usage;            ///< the maxiumum cpu usage in instructions for a block
   uint32_t   target_block_cpu_usage_pct;     ///< the target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
   uint64_t   max_block_net_usage;            ///< the maxiumum net usage in instructions for a block
   uint32_t   target_block_net_usage_pct;     ///< the target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling

   uint32_t   max_transaction_lifetime;
   uint32_t   max_transaction_exec_time;
   uint16_t   max_authority_depth;
   uint16_t   max_inline_depth;
   uint32_t   max_inline_action_size;
   uint32_t   max_generated_transaction_count;

   static chain_config get_median_values( vector<chain_config> votes );

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config& c ) {
      return out << "Per-Signature CPU Usage: " << c.per_signature_cpu_usage << ", "
                 << "Max Transaction CPU Usage: " << c.max_transaction_cpu_usage << ", "
                 << "Max Transaction Net Usage: " << c.max_transaction_net_usage << ", "
                 << "Max Block CPU Usage: " << c.max_block_cpu_usage << ", "
                 << "Target Block CPU Usage Percent: " << c.target_block_cpu_usage_pct << ", "
                 << "Max Block NET Usage: " << c.max_block_net_usage << ", "
                 << "Target Block NET Usage Percent: " << ((double)c.target_block_net_usage_pct / (double)config::percent_1) << "%, "
                 << "Max Transaction Lifetime: " << ((double)c.max_transaction_lifetime / (double)config::percent_1) << "%, "
                 << "Max Authority Depth: " << c.max_authority_depth << ", "
                 << "Max Inline Depth: " << c.max_inline_depth << ", "
                 << "Max Inline Action Size: " << c.max_inline_action_size << ", "
                 << "Max Generated Transaction Count: " << c.max_generated_transaction_count << "\n";
   }
};
       bool operator==(const chain_config& a, const chain_config& b);
inline bool operator!=(const chain_config& a, const chain_config& b) { return !(a == b); }

} } // namespace evt::chain

FC_REFLECT(evt::chain::chain_config,
           (per_signature_cpu_usage)
           (max_transaction_cpu_usage)(max_transaction_net_usage)
           (max_block_cpu_usage)(target_block_cpu_usage_pct)
           (max_block_net_usage)(target_block_net_usage_pct)
           (max_transaction_lifetime)(max_transaction_exec_time)(max_authority_depth)
           (max_inline_depth)(max_inline_action_size)(max_generated_transaction_count)
)