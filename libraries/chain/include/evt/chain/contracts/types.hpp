/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/asset.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/group.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

using type_name       = string;
using field_name      = string;
using action_name     = evt::chain::action_name;
using domain_name     = evt::chain::domain_name;
using token_name      = evt::chain::token_name;
using permission_name = evt::chain::permission_name;
using account_name    = evt::chain::account_name;
using user_id         = public_key_type;
using user_list       = std::vector<public_key_type>;
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
    user_id        issuer;
    time_point_sec issue_time;

    permission_def issue;
    permission_def transfer;
    permission_def manage;
};

struct account_def {
    account_def() = default;
    account_def(const account_name& name)
        : name(name) {}

    account_name   name;
    account_name   creator;
    time_point_sec create_time;
    balance_type   balance;
    balance_type   frozen_balance;

    user_list owner;
};

enum delay_status {
    proposed = 0, executed, cancelled
};

struct delay_def {
    delay_def() = default;

    proposal_name                name;
    public_key_type              proposer;
    delay_status                 status;
    transaction                  trx;
    std::vector<public_key_type> signed_keys;
};

struct newdomain {
    domain_name name;
    user_id     issuer;

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

    static action_name
    get_name() {
        return N(transfer);
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

struct newaccount {
    account_name name;
    user_list    owner;

    static action_name
    get_name() {
        return N(newaccount);
    }
};

struct updateowner {
    account_name name;
    user_list    owner;

    static action_name
    get_name() {
        return N(updateowner);
    }
};

struct transferevt {
    account_name from;
    account_name to;
    balance_type amount;

    static action_name
    get_name() {
        return N(transferevt);
    }
};

struct updateaccount {
    account_name               name;
    fc::optional<user_list>    owner;
    fc::optional<balance_type> balance;
    fc::optional<balance_type> frozen_balance;
};

struct newdelay {
    proposal_name   name;
    public_key_type proposer;
    transaction     trx;

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

    static action_name
    get_name() {
        return N(executedelay);
    }
};

struct updatedelay {
    proposal_name                              name;
    fc::optional<std::vector<public_key_type>> signed_keys;
    fc::optional<delay_status>                 status;
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::type_def, (new_type_name)(type))
FC_REFLECT(evt::chain::contracts::field_def, (name)(type))
FC_REFLECT(evt::chain::contracts::struct_def, (name)(base)(fields))
FC_REFLECT(evt::chain::contracts::action_def, (name)(type))
FC_REFLECT(evt::chain::contracts::abi_def, (types)(structs)(actions))
FC_REFLECT(evt::chain::contracts::token_def, (domain)(name)(owner))
FC_REFLECT(evt::chain::contracts::key_weight, (key)(weight))
FC_REFLECT(evt::chain::contracts::authorizer_weight, (ref)(weight))
FC_REFLECT(evt::chain::contracts::permission_def, (name)(threshold)(authorizers))
FC_REFLECT(evt::chain::contracts::domain_def, (name)(issuer)(issue_time)(issue)(transfer)(manage))
FC_REFLECT(evt::chain::contracts::account_def, (name)(creator)(create_time)(balance)(frozen_balance)(owner))
FC_REFLECT_ENUM(evt::chain::contracts::delay_status, (proposed)(executed)(cancelled))
FC_REFLECT(evt::chain::contracts::delay_def, (name)(proposer)(status)(trx)(signed_keys))

FC_REFLECT(evt::chain::contracts::newdomain, (name)(issuer)(issue)(transfer)(manage))
FC_REFLECT(evt::chain::contracts::issuetoken, (domain)(names)(owner))
FC_REFLECT(evt::chain::contracts::transfer, (domain)(name)(to))
FC_REFLECT(evt::chain::contracts::newgroup, (name)(group))
FC_REFLECT(evt::chain::contracts::updategroup, (name)(group))
FC_REFLECT(evt::chain::contracts::updatedomain, (name)(issue)(transfer)(manage))
FC_REFLECT(evt::chain::contracts::newaccount, (name)(owner))
FC_REFLECT(evt::chain::contracts::updateowner, (name)(owner))
FC_REFLECT(evt::chain::contracts::transferevt, (from)(to)(amount))
FC_REFLECT(evt::chain::contracts::updateaccount, (owner)(balance)(frozen_balance))
FC_REFLECT(evt::chain::contracts::newdelay, (name)(proposer)(trx))
FC_REFLECT(evt::chain::contracts::canceldelay, (name))
FC_REFLECT(evt::chain::contracts::approvedelay, (name)(signatures))
FC_REFLECT(evt::chain::contracts::executedelay, (name))
FC_REFLECT(evt::chain::contracts::updatedelay, (name)(signed_keys)(status))
