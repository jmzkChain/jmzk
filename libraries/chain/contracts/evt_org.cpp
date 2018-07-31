/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_org.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/genesis_state.hpp>

#include <fc/io/json.hpp>

namespace evt { namespace chain { namespace contracts {

void
initialize_evt_org(token_database& token_db, const genesis_state& genesis) {
    // Add reserved everiToken foundation group
    if(!token_db.exists_group(".everiToken")) {
        const char* def = R"(
        {
            "name": ".everiToken",
            "key": "EVT00000000000000000000000000000000000000000000000000",
            "root": {
                "threshold": 19,
                "nodes": [
                    { "weight": 8, "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
                    { "weight": 7, "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
                    { "weight": 5, "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
                    { "weight": 4, "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
                    { "weight": 4, "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" }
                ]
            }
        }
        )";

        auto var = fc::json::from_string(def);
        auto group = var.as<group_def>();
        token_db.add_group(group);
    }

    // Add reserved EVT fungible tokens
    if(!token_db.exists_fungible(evt_sym())) {
        auto evt = fungible_def();
        evt.sym = evt_sym();
        evt.creator = genesis.initial_key;
        evt.create_time = genesis.initial_timestamp;

        auto issue = permission_def();
        issue.name = N(issue);
        issue.threshold = 1;
        issue.authorizers.emplace_back(authorizer_weight(authorizer_ref(group_name(N128(.everiToken))), 1));

        auto manage = permission_def();
        manage.name = N(manage);
        manage.threshold = 0;

        evt.issue = issue;
        evt.manage = manage;

        evt.total_supply = asset(100'000'000'000'000L, evt.sym);
        token_db.add_fungible(evt);

        auto addr = address(N(fungible), N128(EVT), 0);
        token_db.update_asset(addr, evt.total_supply);

        // HACK: Use special address to store current largest symbol id
        auto sym_id_addr  = address(N(fungible), name128(), 0);
        auto sym_id_asset = asset(evt_sym().id(), symbol());
        token_db.update_asset(sym_id_addr, sym_id_asset);
    }

    // Add reserved Pined EVT fungible tokens
    if(!token_db.exists_fungible(pevt_sym())) {
        auto pevt = fungible_def();
        pevt.sym = pevt_sym();
        pevt.creator = genesis.initial_key;
        pevt.create_time = genesis.initial_timestamp;

        auto issue = permission_def();
        issue.name = N(issue);
        issue.threshold = 0;

        auto manage = permission_def();
        manage.name = N(manage);
        manage.threshold = 0;

        pevt.issue = issue;
        pevt.manage = manage;

        pevt.total_supply = asset(0, pevt.sym);

        token_db.add_fungible(pevt);

        // HACK: Use special address to store current largest symbol id
        auto sym_id_addr  = address(N(fungible), name128(), 0);
        auto sym_id_asset = asset(pevt_sym().id(), symbol());
        token_db.update_asset(sym_id_addr, sym_id_asset);
    }
}

}}}  // namespace evt::chain::contracts
