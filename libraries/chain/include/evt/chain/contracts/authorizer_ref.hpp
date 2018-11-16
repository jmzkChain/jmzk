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
    enum ref_type { owner_t, account_t, group_t };

private:
    using owner_type = uint8_t;
    using storage_type = static_variant<owner_type, public_key_type, group_name>;

public:
    authorizer_ref() 
        : storage_(owner_type()) {}

    authorizer_ref(const public_key_type& pkey) 
        : storage_(pkey) {}

    authorizer_ref(const group_name& name)
        : storage_(name) {}

public:
    const public_key_type&
    get_account() const {
        return storage_.get<public_key_type>();
    }

    const group_name&
    get_group() const {
        return storage_.get<group_name>();
    }

    void
    set_account(const public_key_type& pkey) {
        storage_ = pkey;
    }

    void
    set_owner() {
        storage_ = owner_type();
    }

    void
    set_group(const group_name& name) {
        storage_ = name;
    }

    int type() const { return storage_.which(); }

    bool is_account_ref() const { return type() == account_t; }
    bool is_owner_ref() const { return type() == owner_t; }
    bool is_group_ref() const { return type() == group_t; }

    std::string to_string() const;

private:
    storage_type storage_;

private:
    friend struct fc::reflector<authorizer_ref>;
};

}}}  // namespac evt::chain::contracts

namespace fc {

class variant;
void to_variant(const evt::chain::contracts::authorizer_ref& ref, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::contracts::authorizer_ref& ref);

}  // namespace fc

FC_REFLECT(evt::chain::contracts::authorizer_ref, (storage_))