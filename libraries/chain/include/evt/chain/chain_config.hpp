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
    uint32_t net_usage_leeway;

    uint32_t max_transaction_lifetime;         ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
    uint32_t deferred_trx_expiration_window;   ///< the number of seconds after the time a deferred transaction can first execute until it expires
    uint32_t max_transaction_delay;            ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
    uint32_t max_inline_action_size;           ///< maximum allowed size (in bytes) of an inline action
    uint16_t max_inline_action_depth;          ///< recursion depth limit on sending inline actions
    uint16_t max_authority_depth;              ///< recursion depth limit for checking if an authority is satisfied

    void validate() const;

    template <typename Stream>
    friend Stream&
    operator<<(Stream& out, const chain_config& c) {
        return out << "Max Block Net Usage: " << c.max_block_net_usage << ", "
                   << "Target Block Net Usage Percent: " << ((double)c.target_block_net_usage_pct / (double)config::percent_1) << "%, "
                   << "Max Transaction Net Usage: " << c.max_transaction_net_usage << ", "
                   << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "
                   << "Net Usage Leeway: " << c.net_usage_leeway << ", "

                   << "Max Transaction Lifetime: " << c.max_transaction_lifetime << ", "
                   << "Deferred Transaction Expiration Window: " << c.deferred_trx_expiration_window << ", "
                   << "Max Transaction Delay: " << c.max_transaction_delay << ", "
                   << "Max Inline Action Size: " << c.max_inline_action_size << ", "
                   << "Max Inline Action Depth: " << c.max_inline_action_depth << ", "
                   << "Max Authority Depth: " << c.max_authority_depth << "\n";
    }
};

bool operator==(const chain_config& a, const chain_config& b);
inline bool
operator!=(const chain_config& a, const chain_config& b) {
    return !(a == b);
}

}}  // namespace evt::chain

FC_REFLECT(evt::chain::chain_config,
           (max_block_net_usage)(target_block_net_usage_pct)
           (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)
           (max_authority_depth)

)