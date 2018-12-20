/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <any>
#include <type_traits>
#include <evt/chain/types.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

struct action {
public:
    action_name name;
    domain_name domain;
    domain_key  key;
    bytes       data;

public:
    action() {}

    // don't copy cache_ when in copy ctor.
    action(const action& lhs)
        : name(lhs.name)
        , domain(lhs.domain)
        , key(lhs.key)
        , data(lhs.data)
        , cache_() {}

    action(action&& lhs) = default;

    action& operator=(const action& lhs) {
        if(this != &lhs) { // self-assignment check expected
            name   = lhs.name;
            domain = lhs.domain;
            key    = lhs.key;
            data   = lhs.data;
        }
        return *this;
    }

    action& operator=(action&& lhs) noexcept {
        if(this != &lhs) { // self-assignment check expected
            name   = lhs.name;
            domain = lhs.domain;
            key    = lhs.key;
            data   = lhs.data;
            cache_ = std::move(lhs.cache_);
        }
        return *this;
    }

public:
    template<typename T>
    action(const domain_name& domain, const domain_key& key, const T& value)
        : domain(domain)
        , key(key) {
        name   = T::get_name();
        data   = fc::raw::pack(value);
        cache_ = std::make_any<T>(value);
    }

    action(const action_name name, const domain_name& domain, const domain_key& key, const bytes& data)
        : name(name)
        , domain(domain)
        , key(key)
        , data(data) {}

    template<typename T>
    void
    set_data(const T& value) {
        data   = fc::raw::pack(value);
        cache_ = std::make_any<T>(value);
    }

    // if T is a reference, will return the reference to the internal cache value
    // Otherwise if T is a value type, will return new copy. 
    template <typename T>
    T
    data_as() const {
        if(!cache_.has_value()) {
            using raw_type = std::remove_const_t<std::remove_reference_t<T>>;
            EVT_ASSERT(name == raw_type::get_name(), action_type_exception, "action name is not consistent with action struct");
            cache_ = std::make_any<raw_type>(fc::raw::unpack<raw_type>(data));
        }
        // no need to check name here, `any_cast` will throws exception if types don't match
        return std::any_cast<T>(cache_);
    }

private:
    mutable std::any cache_;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::action, (name)(domain)(key)(data))