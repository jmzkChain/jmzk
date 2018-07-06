/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/asset.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>

#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/group.hpp>
#include <evt/chain/contracts/metadata.hpp>

namespace evt { namespace chain { namespace contracts {

using type_name       = string;
using field_name      = string;
using action_name     = evt::chain::action_name;
using domain_name     = evt::chain::domain_name;
using domian_key      = evt::chain::domain_key;
using token_name      = evt::chain::token_name;
using permission_name = evt::chain::permission_name;
using account_name    = evt::chain::account_name;
using fungible_name   = evt::chain::fungible_name;
using user_id         = evt::chain::user_id;
using user_list       = std::vector<user_id>;
using group_name      = evt::chain::group_name;
using group_key       = public_key_type;
using group_def       = group;
using balance_type    = evt::chain::asset;

struct type_def {
    type_def() = default;
    type_def(const type_name& new_type_name, const type_name& type)
        : new_type_name(new_type_name)
        , type(type) {}

    type_name new_type_name;
    type_name type;
};

struct field_def {
    field_def() = default;
    field_def(const field_name& name, const type_name& type)
        : name(name)
        , type(type) {}

    field_name name;
    type_name  type;

    bool
    operator==(const field_def& other) const {
        return std::tie(name, type) == std::tie(other.name, other.type);
    }
};

struct struct_def {
    struct_def() = default;
    struct_def(const type_name& name, const type_name& base, const vector<field_def>& fields)
        : name(name)
        , base(base)
        , fields(fields) {}

    type_name         name;
    type_name         base;
    vector<field_def> fields;

    bool
    operator==(const struct_def& other) const {
        return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
    }
};

struct action_def {
    action_def() = default;
    action_def(const action_name& name, const type_name& type)
        : name(name)
        , type(type) {}

    action_name name;
    type_name   type;
};

struct abi_def {
    abi_def() = default;
    abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<action_def>& actions)
        : types(types)
        , structs(structs)
        , actions(actions) {}

    vector<type_def>   types;
    vector<struct_def> structs;
    vector<action_def> actions;
};

struct token_def {
    token_def() = default;
    token_def(domain_name domain, token_name name, user_list owner)
        : domain(domain)
        , name(name)
        , owner(owner) {}

    domain_name domain;
    token_name  name;
    user_list   owner;

    meta_list metas;
};

struct key_weight {
    public_key_type key;
    weight_type     weight;
};

struct authorizer_weight {
    authorizer_weight() = default;
    authorizer_weight(authorizer_ref ref, weight_type weight)
        : ref(ref)
        , weight(weight) {}

    authorizer_ref ref;
    weight_type    weight;
};

struct permission_def {
    permission_def() = default;
    permission_def(permission_name name, uint32_t threshold, const vector<authorizer_weight>& authorizers)
        : name(name)
        , threshold(threshold)
        , authorizers(authorizers) {}

    permission_name           name;
    uint32_t                  threshold;
    vector<authorizer_weight> authorizers;
};

struct domain_def {
    domain_def() = default;
    domain_def(const domain_name& name)
        : name(name) {}

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

enum delay_status {
    proposed = 0, executed, failed, cancelled
};

struct delay_def {
    delay_def() = default;

    proposal_name                        name;
    public_key_type                      proposer;
    fc::enum_type<uint8_t, delay_status> status;
    transaction                          trx;
    flat_set<public_key_type>            signed_keys;
};

struct newdomain {
    domain_name name;
    user_id     creator;

    permission_def issue;
    permission_def transfer;
    permission_def manage;

    static action_name
    get_name() {
        return N(newdomain);
    }
};

struct issuetoken {
    domain_name             domain;
    std::vector<token_name> names;
    user_list               owner;

    static action_name
    get_name() {
        return N(issuetoken);
    }
};

struct transfer {
    domain_name domain;
    token_name  name;
    user_list   to;
    string      memo;

    static action_name
    get_name() {
        return N(transfer);
    }
};

struct destroytoken {
    domain_name             domain;
    token_name              name;

    static action_name
    get_name() {
        return N(destroytoken);
    }
};

struct newgroup {
    group_name name;
    group_def  group;

    static action_name
    get_name() {
        return N(newgroup);
    }
};

struct updategroup {
    group_name name;
    group_def  group;

    static action_name
    get_name() {
        return N(updategroup);
    }
};

struct updatedomain {
    domain_name name;

    fc::optional<permission_def> issue;
    fc::optional<permission_def> transfer;
    fc::optional<permission_def> manage;

    static action_name
    get_name() {
        return N(updatedomain);
    }
};

struct newfungible {
    symbol        sym;
    user_id       creator;

    permission_def issue;
    permission_def manage;

    asset total_supply;

    static action_name
    get_name() {
        return N(newfungible);
    }
};

struct updfungible {
    symbol sym;

    fc::optional<permission_def> issue;
    fc::optional<permission_def> manage;

    static action_name
    get_name() {
        return N(updfungible);
    }
};

struct issuefungible {
    public_key_type address;
    asset           number;
    string          memo;

    static action_name
    get_name() {
        return N(issuefungible);
    }
};

struct transferft {
    public_key_type from;
    public_key_type to;
    asset           number;
    string          memo;

    static action_name
    get_name() {
        return N(transferft);
    }
};

struct evt2pevt {
    public_key_type from;
    public_key_type to;
    asset           number;
    string          memo;

    static action_name
    get_name() {
        return N(evt2pevt);
    }
};

struct addmeta {
    meta_key       key;
    meta_value     value;
    authorizer_ref creator;

    static action_name
    get_name() {
        return N(addmeta);
    }
};

struct newdelay {
    proposal_name               name;
    user_id                     proposer;
    transaction                 trx;

    static action_name
    get_name() {
        return N(newdelay);
    }
};

struct canceldelay {
    proposal_name name;

    static action_name
    get_name() {
        return N(canceldelay);
    }
};

struct approvedelay {
    proposal_name               name;
    std::vector<signature_type> signatures;

    static action_name
    get_name() {
        return N(approvedelay);
    }
};

struct executedelay {
    proposal_name name;
    user_id       executor;

    static action_name
    get_name() {
        return N(executedelay);
    }
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::type_def, (new_type_name)(type));
FC_REFLECT(evt::chain::contracts::field_def, (name)(type));
FC_REFLECT(evt::chain::contracts::struct_def, (name)(base)(fields));
FC_REFLECT(evt::chain::contracts::action_def, (name)(type));
FC_REFLECT(evt::chain::contracts::abi_def, (types)(structs)(actions));
FC_REFLECT(evt::chain::contracts::token_def, (domain)(name)(owner)(metas));
FC_REFLECT(evt::chain::contracts::key_weight, (key)(weight));
FC_REFLECT(evt::chain::contracts::authorizer_weight, (ref)(weight));
FC_REFLECT(evt::chain::contracts::permission_def, (name)(threshold)(authorizers));
FC_REFLECT(evt::chain::contracts::domain_def, (name)(creator)(create_time)(issue)(transfer)(manage)(metas));
FC_REFLECT(evt::chain::contracts::fungible_def, (sym)(creator)(create_time)(issue)(manage)(total_supply)(current_supply)(metas));
FC_REFLECT_ENUM(evt::chain::contracts::delay_status, (proposed)(executed)(failed)(cancelled));
FC_REFLECT(evt::chain::contracts::delay_def, (name)(proposer)(status)(trx)(signed_keys));

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
FC_REFLECT(evt::chain::contracts::newdelay, (name)(proposer)(trx));
FC_REFLECT(evt::chain::contracts::canceldelay, (name));
FC_REFLECT(evt::chain::contracts::approvedelay, (name)(signatures));
FC_REFLECT(evt::chain::contracts::executedelay, (name)(executor));
