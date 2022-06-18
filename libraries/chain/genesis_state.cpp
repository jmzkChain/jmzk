/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/genesis_state.hpp>
#include <jmzk/chain/contracts/types.hpp>
#include <fc/io/json.hpp>

namespace jmzk { namespace chain {

namespace internal {

using namespace contracts;

group_def
get_jmzk_org() {

#ifndef MAINNET_BUILD
    const char* def = R"(
    {
        "name": ".jmzkChain",
        "key": "jmzk00000000000000000000000000000000000000000000000000",
        "root": {
            "threshold": 1,
            "nodes": [
                { "weight": 1, "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
                { "weight": 1, "key": "jmzk7edNeLHSdfmhMTUZd3o3pTBoyPRZ4fjrKU74FxJR9NgZgNZK6J" }
            ]
        }
    }
    )";
#else
    const char* def = R"(
    {
        "name": ".jmzkChain",
        "key": "jmzk00000000000000000000000000000000000000000000000000",
        "root": {
            "threshold": 19,
            "nodes": [
                { "weight": 8, "key": "jmzk6ZVMb3e69umQB4DQErvovx4fpy4ri2qMRmWnCjqCHRvzeWBYix" },
                { "weight": 7, "key": "jmzk8C5q7W6tieUb1z5e9NV9ohWorWKfHykZp46nVaqabNm5xPSpVe" },
                { "weight": 5, "key": "jmzk8PwjEmVji6xtNZdv8pNUuQyDavDyDcCQFDTZHDV4G6Vk9SMJUT" },
                { "weight": 4, "key": "jmzk6J3hLMqwVMpeCcQh74LJhVs9f23HHjr4AZBUTd9GtTMc7dgGeP" },
                { "weight": 4, "key": "jmzk8MSR6xwSoeDPAQDNZBTkDPvVjwEbuuiysMxdcMAz354WVaxCQu" }
            ]
        }
    }
    )";
#endif

    auto var = fc::json::from_string(def);
    return var.as<group_def>();
}

fungible_def_genesis
get_jmzk_sym(const genesis_state& genesis) {
    auto jmzk = fungible_def_genesis();
    jmzk.name = "jmzk";
    jmzk.sym_name = "jmzk";
    jmzk.sym = jmzk_sym();
    jmzk.creator = genesis.initial_key;
    jmzk.create_time = genesis.initial_timestamp;

    auto issue = permission_def();
    issue.name = N(issue);
    issue.threshold = 1;
    auto ref = authorizer_ref();
    ref.set_group(N128(.jmzkChain));
    issue.authorizers.emplace_back(authorizer_weight(ref, 1));

    auto manage = permission_def();
    manage.name = N(manage);
    manage.threshold = 0;

    jmzk.issue  = issue;
    jmzk.manage = manage;

    jmzk.total_supply = asset(100'000'000'000'000L, jmzk.sym);
    return jmzk;
}

fungible_def_genesis
get_pjmzk_sym(const genesis_state& genesis) {
    auto pjmzk = fungible_def_genesis();
    pjmzk.name = "Pinned.jmzk";
    pjmzk.sym_name = "Pjmzk";
    pjmzk.sym = pjmzk_sym();
    pjmzk.creator = genesis.initial_key;
    pjmzk.create_time = genesis.initial_timestamp;

    auto issue = permission_def();
    issue.name = N(issue);
    issue.threshold = 0;

    auto manage = permission_def();
    manage.name = N(manage);
    manage.threshold = 0;

    pjmzk.issue  = issue;
    pjmzk.manage = manage;

    pjmzk.total_supply = asset(0, pjmzk.sym);
    return pjmzk;
}

auto upgrade_ft = [](auto& ftg, bool can_transfer) {
    auto transfer = permission_def();
    transfer.name = N(transfer);
    if(can_transfer) {
        transfer.threshold = 1;
        transfer.authorizers.emplace_back(authorizer_weight(authorizer_ref(), 1));
    }
    else {
        transfer.threshold = 0;
    }

    auto ft         = fungible_def();
    ft.name         = ftg.name;
    ft.sym_name     = ftg.sym_name;
    ft.sym          = ftg.sym;
    ft.creator      = ftg.creator;
    ft.create_time  = ftg.create_time;
    ft.issue        = ftg.issue;
    ft.transfer     = transfer;
    ft.manage       = ftg.manage;
    ft.total_supply = ftg.total_supply;
    ft.metas        = ftg.metas;

    return ft;
};

}  // namespace internal

genesis_state::genesis_state() {
    using namespace internal;

    initial_timestamp = fc::time_point::from_iso_string("2018-05-31T12:00:00");
    initial_key       = fc::variant(jmzk_root_key).as<public_key_type>();

    jmzk_org = get_jmzk_org();
    jmzk     = get_jmzk_sym(*this);
    pjmzk    = get_pjmzk_sym(*this);
}

chain::chain_id_type
genesis_state::compute_chain_id() const {
    digest_type::encoder enc;
    fc::raw::pack(enc, *this);
    return chain_id_type(enc.result());
}

fungible_def
genesis_state::get_jmzk_ft() const {
    using namespace internal;
    return upgrade_ft(jmzk, true);
}

fungible_def
genesis_state::get_pjmzk_ft() const {
    using namespace internal;
    return upgrade_ft(pjmzk, false);
}

}}  // namespace jmzk::chain