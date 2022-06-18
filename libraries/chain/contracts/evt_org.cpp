/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/jmzk_org.hpp>
#include <jmzk/chain/token_database.hpp>
#include <jmzk/chain/genesis_state.hpp>

namespace jmzk { namespace chain { namespace contracts {

void
initialize_jmzk_org(token_database& tokendb, const genesis_state& genesis) {
    // Add reserved jmzkChain foundation group
    if(!tokendb.exists_token(token_type::group, std::nullopt, N128(.jmzkChain))) {
        auto v = make_db_value(genesis.jmzk_org);
        tokendb.put_token(token_type::group, action_op::add, std::nullopt, N128(.jmzkChain), v.as_string_view());
    }

    // Add reserved jmzk & Pjmzk fungible tokens
    if(!tokendb.exists_token(token_type::fungible, std::nullopt, jmzk_SYM_ID)) {
        assert(!tokendb.exists_token(token_type::fungible, std::nullopt, Pjmzk_SYM_ID));

        auto v = make_db_value(genesis.get_jmzk_ft());
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, jmzk_SYM_ID, v.as_string_view());

        auto v2 = make_db_value(genesis.get_pjmzk_ft());
        tokendb.put_token(token_type::fungible, action_op::add, std::nullopt, Pjmzk_SYM_ID, v2.as_string_view());

        auto addr = address(N(.fungible), name128::from_number(jmzk_SYM_ID), 0);
        auto prop = property_stakes(property {
                        .amount = genesis.jmzk.total_supply.amount(),
                        .frozen_amount = 0,
                        .sym = jmzk_sym(),
                        .created_at = genesis.initial_timestamp.sec_since_epoch(),
                        .created_index = 0
                    });
        auto v3 = make_db_value(prop);
        tokendb.put_asset(addr, jmzk_SYM_ID, v3.as_string_view());
    }
}

void
update_jmzk_org(token_database& tokendb, const genesis_state& genesis) {
    auto s = tokendb.new_savepoint_session();

    auto v = make_db_value(genesis.get_jmzk_ft());
    tokendb.put_token(token_type::fungible, action_op::update, std::nullopt, jmzk_SYM_ID, v.as_string_view());

    auto v2 = make_db_value(genesis.get_pjmzk_ft());
    tokendb.put_token(token_type::fungible, action_op::update, std::nullopt, Pjmzk_SYM_ID, v2.as_string_view());
}

}}}  // namespace jmzk::chain::contracts
