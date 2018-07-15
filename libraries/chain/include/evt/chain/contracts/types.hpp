/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/address.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/group.hpp>
#include <evt/chain/contracts/metadata.hpp>

namespace evt { namespace chain { namespace contracts {

#define EVT_ACTION_NAME(typename)  \
    static constexpr action_name   \
    get_name() {                   \
        return N(typename);        \
    }

using domain_name     = evt::chain::domain_name;
using domian_key      = evt::chain::domain_key;
using token_name      = evt::chain::token_name;
using permission_name = evt::chain::permission_name;
using account_name    = evt::chain::account_name;
using fungible_name   = evt::chain::fungible_name;
using user_id         = evt::chain::public_key_type;
using user_list       = std::vector<user_id>;
using group_name      = evt::chain::group_name;
using group_key       = evt::chain::address;
using group_def       = group;
using balance_type    = evt::chain::asset;
using address_type    = evt::chain::address;
using address_list    = std::vector<address_type>;

struct token_def {
    token_def() = default;
    token_def(const domain_name& domain, const token_name& name, const address_list& owner)
        : domain(domain)
        , name(name)
        , owner(owner) {}

    domain_name  domain;
    token_name   name;
    address_list owner;

    meta_list metas;
};

struct key_weight {
    public_key_type key;
    weight_type     weight;
};

struct authorizer_weight {
    authorizer_weight() = default;
    authorizer_weight(const authorizer_ref& ref, weight_type weight)
        : ref(ref), weight(weight) {}

    authorizer_ref ref;
    weight_type    weight;
};

struct permission_def {
    permission_def() = default;

    permission_name           name;
    uint32_t                  threshold;
    vector<authorizer_weight> authorizers;
};

struct domain_def {
    domain_def() = default;

    domain_name    name;
    user_id        creator;
    time_point_sec create_time;

    permission_def issue;
    permission_def transfer;
    permission_def manage;

    meta_list metas;
};

struct fungible_def {
    fungible_def() = default;

    symbol         sym;
    user_id        creator;
    time_point_sec create_time;

    permission_def issue;
    permission_def manage;

    asset total_supply;
    asset current_supply;
    
    meta_list metas;
};

enum suspend_status {
    proposed = 0, executed, failed, cancelled
};

struct suspend_def {
    suspend_def() = default;

    proposal_name                          name;
    public_key_type                        proposer;
    fc::enum_type<uint8_t, suspend_status> status;
    transaction                            trx;
    flat_set<public_key_type>              signed_keys;
};

struct newdomain {
    domain_name name;
    user_id     creator;

    permission_def issue;
    permission_def transfer;
    permission_def manage;

    EVT_ACTION_NAME(newdomain);
};

struct issuetoken {
    domain_name             domain;
    std::vector<token_name> names;
    address_list            owner;

    EVT_ACTION_NAME(issuetoken);
};

struct transfer {
    domain_name  domain;
    token_name   name;
    address_list to;
    string       memo;

    EVT_ACTION_NAME(transfer);
};

struct destroytoken {
    domain_name domain;
    token_name  name;

    EVT_ACTION_NAME(destroytoken);
};

struct newgroup {
    group_name name;
    group_def  group;

    EVT_ACTION_NAME(newgroup);
};

struct updategroup {
    group_name name;
    group_def  group;

    EVT_ACTION_NAME(updategroup);
};

struct updatedomain {
    domain_name name;

    fc::optional<permission_def> issue;
    fc::optional<permission_def> transfer;
    fc::optional<permission_def> manage;

    EVT_ACTION_NAME(updatedomain);
};

struct newfungible {
    symbol        sym;
    user_id       creator;

    permission_def issue;
    permission_def manage;

    asset total_supply;

    EVT_ACTION_NAME(newfungible);
};

struct updfungible {
    symbol sym;

    fc::optional<permission_def> issue;
    fc::optional<permission_def> manage;

    EVT_ACTION_NAME(updfungible);
};

struct issuefungible {
    address_type address;
    asset        number;
    string       memo;

    EVT_ACTION_NAME(issuefungible);
};

struct transferft {
    address_type from;
    address_type to;
    asset        number;
    string       memo;

    EVT_ACTION_NAME(transferft);
};

struct evt2pevt {
    address_type from;
    address_type to;
    asset        number;
    string       memo;

    EVT_ACTION_NAME(evt2pevt);
};

struct addmeta {
    meta_key       key;
    meta_value     value;
    authorizer_ref creator;

    EVT_ACTION_NAME(addmeta);
};

struct newsuspend {
    proposal_name name;
    user_id       proposer;
    transaction   trx;

    EVT_ACTION_NAME(newsuspend);
};

struct cancelsuspend {
    proposal_name name;

    EVT_ACTION_NAME(cancelsuspend);
};

struct aprvsuspend {
    proposal_name               name;
    std::vector<signature_type> signatures;

    EVT_ACTION_NAME(aprvsuspend);
};

struct execsuspend {
    proposal_name name;
    user_id       executor;

    EVT_ACTION_NAME(execsuspend);
};

struct paycharge {
    address  payer;
    uint64_t charge;

    EVT_ACTION_NAME(paycharge);
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::token_def, (domain)(name)(owner)(metas));
FC_REFLECT(evt::chain::contracts::key_weight, (key)(weight));
FC_REFLECT(evt::chain::contracts::authorizer_weight, (ref)(weight));
FC_REFLECT(evt::chain::contracts::permission_def, (name)(threshold)(authorizers));
FC_REFLECT(evt::chain::contracts::domain_def, (name)(creator)(create_time)(issue)(transfer)(manage)(metas));
FC_REFLECT(evt::chain::contracts::fungible_def, (sym)(creator)(create_time)(issue)(manage)(total_supply)(current_supply)(metas));
FC_REFLECT_ENUM(evt::chain::contracts::suspend_status, (proposed)(executed)(failed)(cancelled));
FC_REFLECT(evt::chain::contracts::suspend_def, (name)(proposer)(status)(trx)(signed_keys));

FC_REFLECT(evt::chain::contracts::newdomain, (name)(creator)(issue)(transfer)(manage));
FC_REFLECT(evt::chain::contracts::issuetoken, (domain)(names)(owner));
FC_REFLECT(evt::chain::contracts::transfer, (domain)(name)(to)(memo));
FC_REFLECT(evt::chain::contracts::destroytoken, (domain)(name));
FC_REFLECT(evt::chain::contracts::newgroup, (name)(group));
FC_REFLECT(evt::chain::contracts::updategroup, (name)(group));
FC_REFLECT(evt::chain::contracts::updatedomain, (name)(issue)(transfer)(manage));
FC_REFLECT(evt::chain::contracts::newfungible, (sym)(creator)(issue)(manage)(total_supply));
FC_REFLECT(evt::chain::contracts::updfungible, (sym)(issue)(manage));
FC_REFLECT(evt::chain::contracts::issuefungible, (address)(number)(memo));
FC_REFLECT(evt::chain::contracts::transferft, (from)(to)(number)(memo));
FC_REFLECT(evt::chain::contracts::evt2pevt, (from)(to)(number)(memo));
FC_REFLECT(evt::chain::contracts::addmeta, (key)(value)(creator));
FC_REFLECT(evt::chain::contracts::newsuspend, (name)(proposer)(trx));
FC_REFLECT(evt::chain::contracts::cancelsuspend, (name));
FC_REFLECT(evt::chain::contracts::aprvsuspend, (name)(signatures));
FC_REFLECT(evt::chain::contracts::execsuspend, (name)(executor));
FC_REFLECT(evt::chain::contracts::paycharge, (payer)(charge));