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
    if(!tokendb.exists_token(token_type::fungible, std::nullopt, EVT_SYM_ID)) {
        assert(!tokendb.exists_token(token_type::fungible, std::nullopt, PEVT_SYM_ID));

        auto v = make_db_value(genesis.get_evt_ft());
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, EVT_SYM_ID, v.as_string_view());

        auto v2 = make_db_value(genesis.get_pevt_ft());
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, PEVT_SYM_ID, v2.as_string_view());

        auto addr = address(N(.fungible), name128::from_number(EVT_SYM_ID), 0);
        auto prop = property_stakes(property {
                        .amount = genesis.evt.total_supply.amount(),
                        .sym = evt_sym(),
                        .created_at = genesis.initial_timestamp.sec_since_epoch(),
                        .created_index = 0
                    });
        auto v3 = make_db_value(prop);
        tokendb.put_asset(addr, EVT_SYM_ID, v3.as_string_view());
    }
}

void
update_evt_org(token_database& tokendb, const genesis_state& genesis) {
    auto s = tokendb.new_savepoint_session();

    auto v = make_db_value(genesis.get_evt_ft());
    tokendb.put_token(token_type::fungible, action_op::update, std::nullopt, EVT_SYM_ID, v.as_string_view());

    auto v2 = make_db_value(genesis.get_pevt_ft());
    tokendb.put_token(token_type::fungible, action_op::update, std::nullopt, PEVT_SYM_ID, v2.as_string_view());
}

}}}  // namespace evt::chain::contracts
