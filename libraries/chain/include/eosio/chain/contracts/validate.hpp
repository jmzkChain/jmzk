/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/contracts/types.hpp>

namespace eosio { namespace chain { namespace contracts {

inline bool 
validate(const permission_def &permission) {
    uint32_t total_weight = 0;
    const contracts::group_weight* prev = nullptr;
    for(const auto& gw : permission.groups) {
        if(!prev) {
            prev = &gw;
        }
        else if(prev->id < gw.id) {
            return false;
        }
        total_weight += gw.weight;
    }
    return total_weight >= permission.threshold;
}

inline bool
validate(const group_def &group) {
    uint32_t total_weight = 0;
    const key_weight* prev = nullptr;
    for(const auto& kw : group.keys) {
        if(!prev) {
            prev = &kw;
        }
        else if(prev->key < kw.key) {
            return false;
        }
        total_weight += kw.weight;
    }
    return total_weight >= group.threshold;
}

}}}  // namespace evt::chain::contracts