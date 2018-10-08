/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>

enum class asset_type {
    tokens = 0, fungible = 1
};

enum class lock_status {
    proposed = 0, succeed, failed
};

struct locknft_def {
    domain_name             domain;
    std::vector<token_name> names;
};

struct lockft_def {
    address from;
    asset   amount;
};

struct lockasset_def {
    fc::enum_type<uint8_t, asset_type> type;
    fc::optional<locknft_def>          tokens;
    fc::optional<lockft_def>           fungible;
};

enum class lock_type {
    cond_keys = 0
};

struct lock_condkeys {
    uint16_t                     threshold;
    std::vector<public_key_type> cond_keys;
};

struct lock_condition {

};

struct lock_def {
    proposal_name                       name;
    user_id                             proposer;
    fc::enum_type<uint8_t, lock_status> status;

    time_point_sec             unlock_time;
    time_point_sec             deadline;
    std::vector<lockasset_def> assets;
    
    lock_cond_t          condition;
    std::vector<address> succeed;
    std::vector<address> failed;

    flat_set<public_key_type> signed_keys;
};