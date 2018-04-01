#pragma once

#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace eosio { namespace chain { namespace contracts {

using namespace boost::multiprecision;

template<size_t Size>
using uint_t = number<cpp_int_backend<Size, Size, unsigned_magnitude, unchecked, void> >;
template<size_t Size>
using int_t = number<cpp_int_backend<Size, Size, signed_magnitude, unchecked, void> >;

using uint8     = uint_t<8>;
using uint16    = uint_t<16>;
using uint32    = uint_t<32>;
using uint64    = uint_t<64>;

using fixed_string32    = fc::fixed_string<fc::array<uint64,4>>;
using fixed_string16    = fc::fixed_string<>;
using type_name         = string;
using field_name        = string;
using table_name        = name;
using action_name       = eosio::chain::action_name;
using domain_name       = eosio::chain::domain_name;
using token_name        = eosio::chain::token_name;
using user_id           = fc::crypto::public_key;
using user_list         = std::vector<fc::crypto::public_key>;
using group_key         = fc::crypto::public_key;
using group_id          = uint128_t;
using permission_name   = name;


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

struct group_def {
    group_def() = default;

    group_id                id;
    group_key               key;
    uint32                  threshold;
    vector<key_weight>      keys;
};

// Special: id == 0 means owner
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
    permission_def(permission_name name, uint32 threshold, const vector<group_weight>& groups)
    : name(name), threshold(threshold), groups(groups)
    {}

    permission_name         name;
    uint32                  threshold;
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


struct newdomain {
    domain_name             name;
    user_id                 issuer;

    permission_def          issue;
    permission_def          transfer;
    permission_def          manage;

    std::vector<group_def>  groups;

    static account_name get_account() {
        return config::system_account_name;
    }

    static action_name get_name() {
        return N(newdomain);
    }
};

struct issuetoken {
    domain_name             domain;
    std::vector<token_name> names;
    user_list               owner;

    static account_name get_account() {
        return config::system_account_name;
    }

    static action_name get_name() {
        return N(issuetoken);
    }
};

struct transfertoken {
    domain_name             domain;
    token_name              name;
    user_list               to;

    static account_name get_account() {
        return config::system_account_name;
    }

    static action_name get_name() {
        return N(transfertoken);
    }
};

struct updategroup {
    group_id                id;
    uint32                  threshold;
    vector<key_weight>      keys;

    static account_name get_account() {
        return config::system_account_name;
    }

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

    static account_name get_account() {
        return config::system_account_name;
    }

    static action_name get_name() {
        return N(newdomain);
    }
};

} } } /// namespace eosio::chain::contracts

FC_REFLECT( eosio::chain::contracts::type_def                         , (new_type_name)(type) )
FC_REFLECT( eosio::chain::contracts::field_def                        , (name)(type) )
FC_REFLECT( eosio::chain::contracts::struct_def                       , (name)(base)(fields) )
FC_REFLECT( eosio::chain::contracts::action_def                       , (name)(type) )
FC_REFLECT( eosio::chain::contracts::abi_def                          , (types)(structs)(actions) )
FC_REFLECT( eosio::chain::contracts::token_def                        , (domain)(name)(owner) )
FC_REFLECT( eosio::chain::contracts::group_def                        , (key)(threshold)(keys) )
FC_REFLECT( eosio::chain::contracts::group_weight                     , (id)(weight) )
FC_REFLECT( eosio::chain::contracts::permission_def                   , (name)(threshold)(groups) )
FC_REFLECT( eosio::chain::contracts::domain_def                       , (name)(issuer)(issue_time)(issue)(transfer)(manage) )

FC_REFLECT( eosio::chain::contracts::newdomain                        , (name)(issuer)(issue)(transfer)(manage)(groups))
FC_REFLECT( eosio::chain::contracts::issuetoken                       , (domain)(names)(owner) )
FC_REFLECT( eosio::chain::contracts::transfertoken                    , (domain)(name)(to) )
FC_REFLECT( eosio::chain::contracts::updategroup                      , (id)(threshold)(keys) )
FC_REFLECT( eosio::chain::contracts::updatedomain                     , (name)(issue)(transfer)(manage)(groups))