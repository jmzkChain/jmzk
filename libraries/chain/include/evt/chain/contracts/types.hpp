/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/chain_config.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/group_id.hpp>

namespace evt { namespace chain { namespace contracts {

using type_name         = string;
using field_name        = string;
using action_name       = evt::chain::action_name;
using domain_name       = evt::chain::domain_name;
using token_name        = evt::chain::token_name;
using permission_name   = evt::chain::permission_name;
using account_name      = evt::chain::account_name;
using user_id           = fc::crypto::public_key;
using user_list         = std::vector<fc::crypto::public_key>;
using group_key         = fc::crypto::public_key;
using balance_type      = int64_t;


struct type_def {
   type_def() = default;
   type_def(const type_name& new_type_name, const type_name& type)
   :new_type_name(new_type_name), type(type)
   {}

   type_name   new_type_name;
   type_name   type;
};

struct field_def {
   field_def() = default;
   field_def(const field_name& name, const type_name& type)
   :name(name), type(type)
   {}

   field_name name;
   type_name  type;

   bool operator==(const field_def& other) const {
      return std::tie(name, type) == std::tie(other.name, other.type);
   }
};

struct struct_def {
   struct_def() = default;
   struct_def(const type_name& name, const type_name& base, const vector<field_def>& fields)
   :name(name), base(base), fields(fields)
   {}

   type_name            name;
   type_name            base;
   vector<field_def>    fields;

   bool operator==(const struct_def& other) const {
      return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
   }
};

struct action_def {
   action_def() = default;
   action_def(const action_name& name, const type_name& type)
   :name(name), type(type)
   {}

   action_name name;
   type_name type;
};

struct abi_def {
   abi_def() = default;
   abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<action_def>& actions)
   :types(types), structs(structs), actions(actions)
   {}

   vector<type_def>     types;
   vector<struct_def>   structs;
   vector<action_def>   actions;
};

struct token_def {
    token_def() = default;
    token_def(domain_name domain, token_name name, user_list owner)
    : domain(domain), name(name), owner(owner)
    {}

    domain_name             domain;
    token_name              name;
    user_list               owner;
};

struct key_weight {
   public_key_type key;
   weight_type     weight;
};

struct group_def {
    group_def() = default;

    group_id                id;
    group_key               key;
    uint32_t                threshold;
    vector<key_weight>      keys;
};

// Special: id.empty() == true means owner
struct group_weight {
    group_weight() = default;
    group_weight(group_id id, weight_type weight)
    : id(id), weight(weight)
    {}

    group_id                id;
    weight_type             weight;
};

struct permission_def {
    permission_def() = default;
    permission_def(permission_name name, uint32_t threshold, const vector<group_weight>& groups)
    : name(name), threshold(threshold), groups(groups)
    {}

    permission_name         name;
    uint32_t                threshold;
    vector<group_weight>    groups;
};

struct domain_def {
    domain_def() = default;
    domain_def(const domain_name& name)
    :name(name)
    {}

    domain_name             name;
    user_id                 issuer;
    time_point_sec          issue_time;

    permission_def          issue;
    permission_def          transfer;
    permission_def          manage;
};

struct account_def {
    account_def() = default;
    account_def(const account_name& name)
    :name(name)
    {}

    account_name            name;
    account_name            creator;
    balance_type            balance;
    balance_type            frozen_balance;

    permission_def          owner;
};

struct newdomain {
    domain_name             name;
    user_id                 issuer;

    permission_def          issue;
    permission_def          transfer;
    permission_def          manage;

    std::vector<group_def>  groups;

    static action_name get_name() {
        return N(newdomain);
    }
};

struct issuetoken {
    domain_name             domain;
    std::vector<token_name> names;
    user_list               owner;

    static action_name get_name() {
        return N(issuetoken);
    }
};

struct transfer {
    domain_name             domain;
    token_name              name;
    user_list               to;

    static action_name get_name() {
        return N(transfer);
    }
};

struct updategroup {
    group_id                id;
    uint32_t                threshold;
    vector<key_weight>      keys;

    static action_name get_name() {
        return N(updategroup);
    }
};

struct updatedomain {
    domain_name                     name;
    
    fc::optional<permission_def>    issue;
    fc::optional<permission_def>    transfer;
    fc::optional<permission_def>    manage;

    std::vector<group_def>          groups;

    static action_name get_name() {
        return N(updatedomain);
    }
};

struct newaccount {
    account_name                    name;
    account_name                    creator;
    permission_def                  owner;
};

struct updateowner {
    account_name                    name;
    permission_def                  owner;
};

} } } /// namespace evt::chain::contracts

FC_REFLECT( evt::chain::contracts::type_def                         , (new_type_name)(type) )
FC_REFLECT( evt::chain::contracts::field_def                        , (name)(type) )
FC_REFLECT( evt::chain::contracts::struct_def                       , (name)(base)(fields) )
FC_REFLECT( evt::chain::contracts::action_def                       , (name)(type) )
FC_REFLECT( evt::chain::contracts::abi_def                          , (types)(structs)(actions) )
FC_REFLECT( evt::chain::contracts::token_def                        , (domain)(name)(owner) )
FC_REFLECT( evt::chain::contracts::key_weight                       , (key)(weight) )
FC_REFLECT( evt::chain::contracts::group_def                        , (id)(key)(threshold)(keys) )
FC_REFLECT( evt::chain::contracts::group_weight                     , (id)(weight) )
FC_REFLECT( evt::chain::contracts::permission_def                   , (name)(threshold)(groups) )
FC_REFLECT( evt::chain::contracts::domain_def                       , (name)(issuer)(issue_time)(issue)(transfer)(manage) )
FC_REFLECT( evt::chain::contracts::account_def                      , (name)(creator)(balance)(frozen_balance)(owner) )

FC_REFLECT( evt::chain::contracts::newdomain                        , (name)(issuer)(issue)(transfer)(manage)(groups))
FC_REFLECT( evt::chain::contracts::issuetoken                       , (domain)(names)(owner) )
FC_REFLECT( evt::chain::contracts::transfer                         , (domain)(name)(to) )
FC_REFLECT( evt::chain::contracts::updategroup                      , (id)(threshold)(keys) )
FC_REFLECT( evt::chain::contracts::updatedomain                     , (name)(issue)(transfer)(manage)(groups))
FC_REFLECT( evt::chain::contracts::newaccount                       , (name)(creator)(owner) )
FC_REFLECT( evt::chain::contracts::updateowner                      , (name)(owner) )
