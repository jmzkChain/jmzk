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
        auto strkey = (std::string)public_key_type();
        printf("%s\n", strkey.c_str());

        const char* def = R"(
        {
            "name": ".everiToken",
            "key": "EVT1111111111111111111111111111111114T1Anm",
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
    if(!token_db.exists_fungible("EVT")) {
        auto evt = fungible_def();
        evt.sym = symbol(SY(5,EVT));
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
        evt.current_supply = asset(0, evt.sym);

        token_db.add_fungible(evt);
    }

    // Add reserved Pined EVT fungible tokens
    if(!token_db.exists_fungible("PEVT")) {
        auto pevt = fungible_def();
        pevt.sym = symbol(SY(5,PEVT));
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
        pevt.current_supply = asset(0, pevt.sym);

        token_db.add_fungible(pevt);
    }
}

}}}  // namespace evt::chain::contracts
