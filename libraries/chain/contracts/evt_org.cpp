/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_org.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/genesis_state.hpp>

namespace evt { namespace chain { namespace contracts {

namespace __internal {

template <typename T>
std::string
get_value(const T& v) {
    auto value = std::string();
    auto size  = fc::raw::pack_size(v);
    value.resize(size);

    auto ds = fc::datastream<char*>((char*)value.data(), size);
    fc::raw::pack(ds, v);

    return value;
}

}  // namespace __internal

void
initialize_evt_org(token_database& tokendb, const genesis_state& genesis) {
    using namespace __internal;

    // Add reserved everiToken foundation group
    if(!tokendb.exists_token(token_type::group, std::nullopt, N128(.everiToken))) {
        auto v = get_value(genesis.evt_org);
        tokendb.put_token(token_type::group, action_op::add, std::nullopt, N128(.everiToken), v);
    }

    // Add reserved EVT & PEVT fungible tokens
    if(!tokendb.exists_token(token_type::fungible, std::nullopt, evt_sym().id())) {
        assert(!tokendb.exists_token(token_type::fungible, std::nullopt, pevt_sym().id()));

        auto v = get_value(genesis.evt);
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, evt_sym().id(), v);

        auto v2 = get_value(genesis.pevt);
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, pevt_sym().id(), v2);

        auto addr = address(N(.fungible), name128::from_number(evt_sym().id()), 0);
        auto v3   = get_value(genesis.evt.total_supply);
        tokendb.put_asset(addr, evt_sym(), v3);
    }
}

}}}  // namespace evt::chain::contracts
