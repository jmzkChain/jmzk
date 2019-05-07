/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/name.hpp>
#include <evt/chain/types.hpp>
#include <fc/time.hpp>

#pragma GCC diagnostic ignored "-Wunused-variable"

namespace evt { namespace chain { namespace config {

typedef __uint128_t uint128_t;

const static auto default_blocks_dir_name          = "blocks";
const static auto reversible_blocks_dir_name       = "reversible";
const static auto default_token_database_dir_name  = "tokendb";
const static auto default_reversible_cache_size    = 340*1024*1024ll;  /// 1MB * 340 blocks based on 21 producer BFT delay
const static auto default_reversible_guard_size    = 2*1024*1024ll;    /// 1MB * 2 blocks based on 21 producer BFT delay
const static auto token_database_persisit_filename = "savepoints.log";

const static auto default_state_dir_name        = "state";
const static auto forkdb_filename               = "forkdb.dat";
const static auto default_state_size            = 1*1024*1024*1024ll;
const static auto default_state_guard_size      = 128*1024*1024ll;

const static uint128_t system_account_name = N128(evt);

const static int      block_interval_ms     = 500;
const static int      block_interval_us     = block_interval_ms * 1000;
const static uint64_t block_timestamp_epoch = 946684800000ll;  // epoch is year 2000.

/** Percentages are fixed point with a denominator of 10,000 */
const static int percent_100 = 10000;
const static int percent_1   = 100;

const static uint32_t default_max_block_net_usage                 = 1024 * 1024;     /// at 500ms blocks and 200byte trx, this enables ~10,000 TPS burst
const static uint32_t default_target_block_net_usage_pct          = 10 * percent_1;  /// we target 1000 TPS
const static uint32_t default_max_transaction_net_usage           = default_max_block_net_usage / 2;
const static uint32_t default_base_per_transaction_net_usage      = 12;  // 12 bytes (11 bytes for worst case of transaction_receipt_header + 1 byte for static_variant tag)
const static uint32_t transaction_id_net_usage                    = 32;  // 32 bytes for the size of a transaction id

const static uint32_t default_max_trx_lifetime               = 60*60; // 1 hour
const static uint16_t default_max_auth_depth                 = 6;

// Should be large enough to allow recovery from badly set blockchain parameters without a hard fork 
const static uint32_t fixed_net_overhead_of_packed_trx = 16; 

const static uint32_t default_base_network_charge_factor = 1;
const static uint32_t default_base_storage_charge_factor = 1;
const static uint32_t default_base_cpu_charge_factor     = 10;
const static uint32_t default_global_charge_factor       = 10;

const static uint32_t default_abi_serializer_max_time_ms = 50; ///< default deadline for abi serialization methods

/**
 *  The number of sequential blocks produced by a single producer
 */
const static int producer_repetitions = 12;
const static int max_producers        = 125;

const static size_t maximum_tracked_dpos_confirmations = 1024;  ///<
static_assert(maximum_tracked_dpos_confirmations >= ((max_producers * 2 / 3) + 1) * producer_repetitions, "Settings never allow for DPOS irreversibility");

/**
 * The number of blocks produced per round is based upon all producers having a chance
 * to produce all of their consecutive blocks.
 */
//const static int blocks_per_round = producer_count * producer_repetitions;

const static int irreversible_threshold_percent = 70 * percent_1;

const static int default_evt_link_expired_secs = 20;  // 20s -> total: 40s

}}}  // namespace evt::chain::config

template <typename Number>
Number
EVT_PERCENT(Number value, int percentage) {
    return value * percentage / evt::chain::config::percent_100;
}

template <typename Number>
Number
EVT_PERCENT_CEIL(Number value, uint32_t percentage) {
    return ((value * percentage) + evt::chain::config::percent_100 - evt::chain::config::percent_1)
           / evt::chain::config::percent_100;
}
