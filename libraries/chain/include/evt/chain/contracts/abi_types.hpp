/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

using type_name   = string;
using field_name  = string;
using action_name = evt::chain::action_name;

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

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::type_def, (new_type_name)(type));
FC_REFLECT(evt::chain::contracts::field_def, (name)(type));
FC_REFLECT(evt::chain::contracts::struct_def, (name)(base)(fields));
FC_REFLECT(evt::chain::contracts::action_def, (name)(type));
FC_REFLECT(evt::chain::contracts::abi_def, (types)(structs)(actions));