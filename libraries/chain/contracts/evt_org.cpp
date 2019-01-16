/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_org.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/genesis_state.hpp>

namespace evt { namespace chain { namespace contracts {

void
initialize_evt_org(token_database& tokendb, const genesis_state& genesis) {
    // Add reserved everiToken foundation group
    if(!tokendb.exists_token(token_type::group, std::nullopt, N128(.everiToken))) {
        auto v = make_db_value(genesis.evt_org);
        tokendb.put_token(token_type::group, action_op::add, std::nullopt, N128(.everiToken), v.as_string_view());
    }

    // Add reserved EVT & PEVT fungible tokens
    if(!tokendb.exists_token(token_type::fungible, std::nullopt, evt_sym().id())) {
        assert(!tokendb.exists_token(token_type::fungible, std::nullopt, pevt_sym().id()));

        auto v = make_db_value(genesis.evt);
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, evt_sym().id(), v.as_string_view());

        auto v2 = make_db_value(genesis.pevt);
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, pevt_sym().id(), v2.as_string_view());

        auto addr = address(N(.fungible), name128::from_number(evt_sym().id()), 0);
        auto v3   = make_db_value(genesis.evt.total_supply);
        tokendb.put_asset(addr, evt_sym(), v3.as_string_view());
    }
}

}}}  // namespace evt::chain::contracts
