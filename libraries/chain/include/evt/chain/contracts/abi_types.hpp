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
public:
    type_def() = default;
    type_def(const type_name& new_type_name, const type_name& type)
        : new_type_name(new_type_name)
        , type(type) {}

public:
    type_name new_type_name;
    type_name type;
};

struct field_def {
public:
    field_def() = default;
    field_def(const field_name& name, const type_name& type)
        : name(name)
        , type(type) {}

public:
    field_name name;
    type_name  type;

public:
    bool
    operator==(const field_def& other) const {
        return std::tie(name, type) == std::tie(other.name, other.type);
    }
};

struct struct_def {
public:
    struct_def() = default;
    struct_def(const type_name& name, const type_name& base, const small_vector<field_def, 8>& fields)
        : name(name)
        , base(base)
        , fields(fields) {}

public:
    type_name                  name;
    type_name                  base;
    small_vector<field_def, 8> fields;

    bool
    operator==(const struct_def& other) const {
        return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
    }
};

struct variant_def {
public:
    variant_def() = default;
    variant_def(const type_name& name, const small_vector<field_def, 8>& fields)
        : name(name)
        , fields(fields) {}

public:
    type_name                  name;
    small_vector<field_def, 8> fields;

    bool
    operator==(const variant_def& other) const {
        return std::tie(name, fields) == std::tie(other.name, other.fields);
    }
};

struct enum_def {
public:
    enum_def() = default;
    enum_def(const type_name& name, const type_name& integer, const small_vector<field_name, 8>& fields)
        : name(name)
        , integer(integer)
        , fields(fields) {}

public:
    type_name                   name;
    type_name                   integer;
    small_vector<field_name, 8> fields;

    bool
    operator==(const enum_def& other) const {
        return std::tie(name, integer, fields) == std::tie(other.name, other.integer, other.fields);
    }
};

struct abi_def {
public:
    abi_def() = default;
    abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<variant_def>& variants, const std::vector<enum_def> enums)
        : types(types)
        , structs(structs)
        , variants(variants)
        , enums(enums) {}

public:
    vector<type_def>    types;
    vector<struct_def>  structs;
    vector<variant_def> variants;
    vector<enum_def>    enums;
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::type_def, (new_type_name)(type));
FC_REFLECT(evt::chain::contracts::field_def, (name)(type));
FC_REFLECT(evt::chain::contracts::struct_def, (name)(base)(fields));
FC_REFLECT(evt::chain::contracts::variant_def, (name)(fields));
FC_REFLECT(evt::chain::contracts::enum_def, (name)(integer)(fields));
FC_REFLECT(evt::chain::contracts::abi_def, (types)(structs)(variants)(enums));
