/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/asset.hpp>

namespace evt { namespace chain {

// represent for property for one symbol in one account
// also records the create time
struct property {
    int64_t  amount;        // amount for asset
    symbol   sym;           // symbol
    uint32_t created_at;    // utc seconds
    uint32_t created_index; // action index at that time
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::property, (amount)(sym)(created_at)(created_index));
