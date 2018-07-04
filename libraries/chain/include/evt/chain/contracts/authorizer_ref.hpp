/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>
#include <fc/array.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

struct authorizer_ref {
public:
    enum ref_type { none = 0, account_t, owner_t, group_t };

public:
    authorizer_ref() = default;

    authorizer_ref(const public_key_type& pkey) : type_(account_t) {
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &pkey, sizeof(pkey));
    }

    authorizer_ref(const group_name& name) : type_(group_t) {
        static_assert(sizeof(group_name) <= sizeof(storage_), "Not fit storage");
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &name, sizeof(name));
    }

public:
    const public_key_type&
    get_account() const {
        return *(public_key_type*)storage_.data;
    }

    const group_name&
    get_group() const {
        return *(group_name*)storage_.data;
    }

    void
    set_account(const public_key_type& pkey) {
        type_ = account_t;
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &pkey, PKEY_SIZE);
    }

    void
    set_owner() {
        type_ = owner_t;
        memset(storage_.data, 0, sizeof(storage_));
    }

    void
    set_group(const group_name& name) {
        type_ = group_t;
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &name, sizeof(name));
    }

    int  type() const { return type_; }
    bool is_account_ref() const { return type_ == account_t; }
    bool is_owner_ref() const { return type_ == owner_t; }
    bool is_group_ref() const { return type_ == group_t; }

private:
    static constexpr size_t PKEY_SIZE = sizeof(public_key_type::storage_type::type_at<0>);

    int  type_;
    fc::array<char, sizeof(public_key_type)> storage_;

    friend struct fc::reflector<authorizer_ref>;
};

}}}  // namespac evt::chain::contracts

namespace fc {

class variant;
void to_variant(const evt::chain::contracts::authorizer_ref& ref, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::contracts::authorizer_ref& ref);

}  // namespace fc

FC_REFLECT(evt::chain::contracts::authorizer_ref, (type_)(storage_))