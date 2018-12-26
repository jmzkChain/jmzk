/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <fc/variant_wrapper.hpp>

#include <evt/chain/address.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/producer_schedule.hpp>

#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/group.hpp>
#include <evt/chain/contracts/metadata.hpp>
#include <evt/chain/contracts/evt_link.hpp>

namespace evt { namespace chain { namespace contracts {

#define EVT_ACTION(typename)       \
    typename() = default;          \
                                   \
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
using symbol_name     = evt::chain::symbol_name;
using symbol_id_type  = evt::chain::symbol_id_type;
using user_id         = evt::chain::public_key_type;
using group_name      = evt::chain::group_name;
using group_key       = evt::chain::address;
using group_def       = group;
using balance_type    = evt::chain::asset;
using address_type    = evt::chain::address;
using address_list    = small_vector<address_type, 4>;
using conf_key        = evt::chain::conf_key;

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

    permission_name                    name;
    uint32_t                           threshold;
    small_vector<authorizer_weight, 4> authorizers;
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

    fungible_name  name;
    symbol_name    sym_name;
    symbol         sym;
    
    user_id        creator;
    time_point_sec create_time;

    permission_def issue;
    permission_def manage;

    asset total_supply;
    
    meta_list metas;
};

enum class suspend_status {
    proposed = 0, executed, failed, cancelled
};

struct suspend_def {
    suspend_def() = default;

    proposal_name                          name;
    public_key_type                        proposer;
    fc::enum_type<uint8_t, suspend_status> status;
    transaction                            trx;
    public_keys_type                       signed_keys;
    signatures_type                        signatures;
};

enum class asset_type {
    tokens = 0, fungible = 1, max_value = 1
};

enum class lock_status {
    proposed = 0, succeed, failed
};

struct locknft_def {
    domain_name                 domain;
    small_vector<token_name, 4> names;
};

struct lockft_def {
    address from;
    asset   amount;
};

using lock_asset = fc::variant_wrapper<asset_type, locknft_def, lockft_def>;

enum class lock_type {
    cond_keys = 0, max_value = 0
};

struct lock_condkeys {
    uint16_t                     threshold;
    std::vector<public_key_type> cond_keys;
};

using lock_condition = fc::variant_wrapper<lock_type, lock_condkeys>;

struct lock_def {
    proposal_name                       name;
    user_id                             proposer;
    fc::enum_type<uint8_t, lock_status> status;

    time_point_sec              unlock_time;
    time_point_sec              deadline;
    small_vector<lock_asset, 4> assets;
    
    lock_condition           condition;
    small_vector<address, 4> succeed;
    small_vector<address, 4> failed;

    flat_set<public_key_type> signed_keys;
};

enum class lock_aprv_type {
    cond_key = 0, max_value = 0
};

using lock_aprvdata = fc::variant_wrapper<lock_aprv_type, void_t>;

struct newdomain {
    domain_name name;
    user_id     creator;

    permission_def issue;
    permission_def transfer;
    permission_def manage;

    EVT_ACTION(newdomain);
};

struct issuetoken {
    domain_name                 domain;
    small_vector<token_name, 4> names;
    address_list                owner;

    EVT_ACTION(issuetoken);
};

struct transfer {
    domain_name  domain;
    token_name   name;
    address_list to;
    string       memo;

    EVT_ACTION(transfer);
};

struct destroytoken {
    domain_name domain;
    token_name  name;

    EVT_ACTION(destroytoken);
};

struct newgroup {
    group_name name;
    group_def  group;

    EVT_ACTION(newgroup);
};

struct updategroup {
    group_name name;
    group_def  group;

    EVT_ACTION(updategroup);
};

struct updatedomain {
    domain_name name;

    optional<permission_def> issue;
    optional<permission_def> transfer;
    optional<permission_def> manage;

    EVT_ACTION(updatedomain);
};

struct newfungible {
    fungible_name name;
    symbol_name   sym_name;
    symbol        sym;
    user_id       creator;

    permission_def issue;
    permission_def manage;

    asset total_supply;

    EVT_ACTION(newfungible);
};

struct updfungible {
    symbol_id_type sym_id;

    optional<permission_def> issue;
    optional<permission_def> manage;

    EVT_ACTION(updfungible);
};

struct issuefungible {
    address_type address;
    asset        number;
    string       memo;

    EVT_ACTION(issuefungible);
};

struct transferft {
    address_type from;
    address_type to;
    asset        number;
    string       memo;

    EVT_ACTION(transferft);
};

struct recycleft {
    address_type address;
    asset        number;
    string       memo;

    EVT_ACTION(recycleft);
};

struct destroyft {
    address_type address;
    asset        number;
    string       memo;

    EVT_ACTION(destroyft);
};

struct evt2pevt {
    address_type from;
    address_type to;
    asset        number;
    string       memo;

    EVT_ACTION(evt2pevt);
};

struct addmeta {
    meta_key       key;
    meta_value     value;
    authorizer_ref creator;

    EVT_ACTION(addmeta);
};

struct newsuspend {
    proposal_name name;
    user_id       proposer;
    transaction   trx;

    EVT_ACTION(newsuspend);
};

struct cancelsuspend {
    proposal_name name;

    EVT_ACTION(cancelsuspend);
};

struct aprvsuspend {
    proposal_name                       name;
    fc::small_vector<signature_type, 4> signatures;

    EVT_ACTION(aprvsuspend);
};

struct execsuspend {
    proposal_name name;
    user_id       executor;

    EVT_ACTION(execsuspend);
};

struct paycharge {
    address  payer;
    uint32_t charge;

    EVT_ACTION(paycharge);
};

struct everipass {
    evt_link link;

    EVT_ACTION(everipass);
};

struct everipay {
    evt_link link;
    address  payee;
    asset    number;

    EVT_ACTION(everipay);
};

struct prodvote {
    account_name producer;
    conf_key     key;
    int64_t      value;

    EVT_ACTION(prodvote);
};

struct updsched {
    vector<producer_key> producers;

    EVT_ACTION(updsched);
};

struct newlock {
    proposal_name name;
    user_id       proposer;

    time_point_sec              unlock_time;
    time_point_sec              deadline;
    small_vector<lock_asset, 4> assets;
    
    lock_condition           condition;
    small_vector<address, 4> succeed;
    small_vector<address, 4> failed;

    EVT_ACTION(newlock);
};

struct aprvlock {
    proposal_name name;
    user_id       approver;
    lock_aprvdata data;

    EVT_ACTION(aprvlock);
};

struct tryunlock {
    proposal_name name;
    user_id       executor;

    EVT_ACTION(tryunlock);
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::token_def, (domain)(name)(owner)(metas));
FC_REFLECT(evt::chain::contracts::key_weight, (key)(weight));
FC_REFLECT(evt::chain::contracts::authorizer_weight, (ref)(weight));
FC_REFLECT(evt::chain::contracts::permission_def, (name)(threshold)(authorizers));
FC_REFLECT(evt::chain::contracts::domain_def, (name)(creator)(create_time)(issue)(transfer)(manage)(metas));
FC_REFLECT(evt::chain::contracts::fungible_def, (name)(sym_name)(sym)(creator)(create_time)(issue)(manage)(total_supply)(metas));
FC_REFLECT_ENUM(evt::chain::contracts::suspend_status, (proposed)(executed)(failed)(cancelled));
FC_REFLECT(evt::chain::contracts::suspend_def, (name)(proposer)(status)(trx)(signed_keys)(signatures));
FC_REFLECT_ENUM(evt::chain::contracts::asset_type, (tokens)(fungible));
FC_REFLECT_ENUM(evt::chain::contracts::lock_status, (proposed)(succeed)(failed));
FC_REFLECT(evt::chain::contracts::locknft_def, (domain)(names));
FC_REFLECT(evt::chain::contracts::lockft_def, (from)(amount));
FC_REFLECT_ENUM(evt::chain::contracts::lock_type, (cond_keys));
FC_REFLECT(evt::chain::contracts::lock_condkeys, (threshold)(cond_keys));
FC_REFLECT(evt::chain::contracts::lock_def, (name)(proposer)(status)(unlock_time)(deadline)(assets)(condition)(succeed)(failed)(signed_keys));
FC_REFLECT_ENUM(evt::chain::contracts::lock_aprv_type, (cond_key));

FC_REFLECT(evt::chain::contracts::newdomain, (name)(creator)(issue)(transfer)(manage));
FC_REFLECT(evt::chain::contracts::issuetoken, (domain)(names)(owner));
FC_REFLECT(evt::chain::contracts::transfer, (domain)(name)(to)(memo));
FC_REFLECT(evt::chain::contracts::destroytoken, (domain)(name));
FC_REFLECT(evt::chain::contracts::newgroup, (name)(group));
FC_REFLECT(evt::chain::contracts::updategroup, (name)(group));
FC_REFLECT(evt::chain::contracts::updatedomain, (name)(issue)(transfer)(manage));
FC_REFLECT(evt::chain::contracts::newfungible, (name)(sym_name)(sym)(creator)(issue)(manage)(total_supply));
FC_REFLECT(evt::chain::contracts::updfungible, (sym_id)(issue)(manage));
FC_REFLECT(evt::chain::contracts::issuefungible, (address)(number)(memo));
FC_REFLECT(evt::chain::contracts::transferft, (from)(to)(number)(memo));
FC_REFLECT(evt::chain::contracts::recycleft, (address)(number)(memo));
FC_REFLECT(evt::chain::contracts::destroyft, (address)(number)(memo));
FC_REFLECT(evt::chain::contracts::evt2pevt, (from)(to)(number)(memo));
FC_REFLECT(evt::chain::contracts::addmeta, (key)(value)(creator));
FC_REFLECT(evt::chain::contracts::newsuspend, (name)(proposer)(trx));
FC_REFLECT(evt::chain::contracts::cancelsuspend, (name));
FC_REFLECT(evt::chain::contracts::aprvsuspend, (name)(signatures));
FC_REFLECT(evt::chain::contracts::execsuspend, (name)(executor));
FC_REFLECT(evt::chain::contracts::paycharge, (payer)(charge));
FC_REFLECT(evt::chain::contracts::everipass, (link));
FC_REFLECT(evt::chain::contracts::everipay, (link)(payee)(number));
FC_REFLECT(evt::chain::contracts::prodvote, (producer)(key)(value));
FC_REFLECT(evt::chain::contracts::updsched, (producers));
FC_REFLECT(evt::chain::contracts::newlock, (name)(proposer)(unlock_time)(deadline)(assets)(condition)(succeed)(failed));
FC_REFLECT(evt::chain::contracts::aprvlock, (name)(approver)(data));
FC_REFLECT(evt::chain::contracts::tryunlock, (name)(executor));
