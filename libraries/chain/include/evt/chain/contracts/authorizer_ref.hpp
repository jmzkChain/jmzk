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
#include <evt/chain/contracts/group_id.hpp>

namespace evt { namespace chain { namespace contracts {

struct authorizer_ref {
public:
    enum ref_type { account_t = 0, group_t };

public:
    authorizer_ref() = default;

    authorizer_ref(const public_key_type& pkey) : type_(account_t) {
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &pkey, sizeof(pkey));
    }

    authorizer_ref(const group_id& gid) : type_(group_t) {
        static_assert(sizeof(group_id) <= sizeof(storage_));
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &gid, sizeof(gid));
    }

public:
    const public_key_type&
    get_account() const {
        return *(public_key_type*)storage_.data;
    }

    const group_id&
    get_group() const {
        return *(group_id*)storage_.data;
    }

    void
    set_account(const public_key_type& pkey) {
        type_ = account_t;
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &pkey, sizeof(pkey));
    }

    void
    set_group(const group_id& gid) {
        type_ = group_t;
        memset(storage_.data, 0, sizeof(storage_));
        memcpy(storage_.data, &gid, sizeof(gid));
    }

    int  type() const { return type_; }
    bool is_account_ref() const { return type_ == account_t; }
    bool is_group_ref() const { return type_ == group_t; }

public:
    int  type_;
    fc::array<char, sizeof(public_key_type)> storage_;
};

}}}  // namespac evt::chain::contracts

namespace fc {

class variant;
void to_variant(const evt::chain::contracts::authorizer_ref& ref, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::contracts::authorizer_ref& ref);

}  // namespace fc

FC_REFLECT(evt::chain::contracts::authorizer_ref, (type_)(storage_))