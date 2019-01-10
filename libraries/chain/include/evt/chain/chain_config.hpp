/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config {
    uint64_t max_block_net_usage;             ///< the maxiumum net usage in instructions for a block
    uint32_t target_block_net_usage_pct;      ///< the target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling
    uint32_t max_transaction_net_usage;       ///< the maximum objectively measured net usage that the chain will allow regardless of account limits
    uint32_t base_per_transaction_net_usage;  ///< the base amount of net usage billed for a transaction to cover incidentals

    uint32_t base_network_charge_factor;
    uint32_t base_storage_charge_factor;
    uint32_t base_cpu_charge_factor;
    uint32_t global_charge_factor;

    uint32_t max_transaction_lifetime;         ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
    uint16_t max_authority_depth;              ///< recursion depth limit for checking if an authority is satisfied

    uint32_t evt_link_expired_secs;

    void validate() const;

    template <typename Stream>
    friend Stream&
    operator<<(Stream& out, const chain_config& c) {
        return out << "Max Block Net Usage: " << c.max_block_net_usage << ", "
                   << "Target Block Net Usage Percent: " << ((double)c.target_block_net_usage_pct / (double)config::percent_1) << "%, "
                   << "Max Transaction Net Usage: " << c.max_transaction_net_usage << ", "
                   << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "

                   << "Base Network Charge Factor: " << c.base_network_charge_factor << ", "
                   << "Base Storage Charge Factor: " << c.base_storage_charge_factor << ", "
                   << "Base CPU Charge Factor: " << c.base_cpu_charge_factor << ", "
                   << "Global Charge Factor: " << c.global_charge_factor << ", "

                   << "Max Transaction Lifetime: " << c.max_transaction_lifetime << ", "
                   << "Max Authority Depth: " << c.max_authority_depth << ", "

                   << "EVT-Link expried secs: " << c.evt_link_expired_secs << "\n";
    }

    friend inline bool
    operator ==(const chain_config& lhs, const chain_config& rhs) {
        return std::tie(lhs.max_block_net_usage,
                        lhs.target_block_net_usage_pct,
                        lhs.max_transaction_net_usage,
                        lhs.base_per_transaction_net_usage,
                        lhs.base_network_charge_factor,
                        lhs.base_storage_charge_factor,
                        lhs.base_cpu_charge_factor,
                        lhs.global_charge_factor,
                        lhs.max_transaction_lifetime,
                        lhs.max_authority_depth,
                        lhs.evt_link_expired_secs
                        )
               ==
               std::tie(lhs.max_block_net_usage,
                        lhs.target_block_net_usage_pct,
                        lhs.max_transaction_net_usage,
                        lhs.base_per_transaction_net_usage,
                        lhs.base_network_charge_factor,
                        lhs.base_storage_charge_factor,
                        lhs.base_cpu_charge_factor,
                        lhs.global_charge_factor,
                        lhs.max_transaction_lifetime,
                        lhs.max_authority_depth,
                        lhs.evt_link_expired_secs
                        );
    };

    friend inline bool
    operator !=(const chain_config& lhs, const chain_config& rhs) {
        return !(lhs == rhs);
    }
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::chain_config,
          (max_block_net_usage)(target_block_net_usage_pct)
          (max_transaction_net_usage)(base_per_transaction_net_usage)
          (base_network_charge_factor)(base_storage_charge_factor)(base_cpu_charge_factor)(global_charge_factor)
          (max_transaction_lifetime)(max_authority_depth)(evt_link_expired_secs)
          );
