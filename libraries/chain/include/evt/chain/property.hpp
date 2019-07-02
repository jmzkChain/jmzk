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
    int64_t  frozen_amount; // frozen amount for asset
    symbol   sym;           // symbol
    uint32_t created_at;    // utc seconds
    uint32_t created_index; // action index at that time
};

enum class stake_type {
    demand = 0, fixed
};

struct stakeshare_def {
    account_name   validator;
    int64_t        units;
    asset          net_value;
    time_point_sec purchase_time;
    stake_type     type;
    int32_t        fixed_hours;
};

struct property_stakes : public property {
    property_stakes() = default;
    property_stakes(const property& lhs) : property(lhs) {}

    std::vector<stakeshare_def> stake_shares;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::property, (amount)(sym)(created_at)(created_index));
FC_REFLECT_ENUM(evt::chain::stake_type, (demand)(fixed));
FC_REFLECT(evt::chain::stakeshare_def, (validator)(units)(net_value)(purchase_time)(type)(fixed_hours));
FC_REFLECT_DERIVED(evt::chain::property_stakes, (evt::chain::property), (stake_shares));
