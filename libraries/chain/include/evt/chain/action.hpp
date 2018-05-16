/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

struct action {
    action_name name;
    domain_name domain;
    domain_key  key;
    bytes       data;

    action() {}

    template <typename T>
    action(const domain_name& domain, const domain_key& key, const T& value)
        : domain(domain)
        , key(key) {
        name = T::get_name();
        data = fc::raw::pack(value);
    }

    action(const action_name name, const domain_name& domain, const domain_key& key, const bytes& data)
        : name(name)
        , domain(domain)
        , key(key)
        , data(data) {}

    template <typename T>
    T
    data_as() const {
        FC_ASSERT(name == T::get_name());
        return fc::raw::unpack<T>(data);
    }
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::action, (name)(domain)(key)(data))