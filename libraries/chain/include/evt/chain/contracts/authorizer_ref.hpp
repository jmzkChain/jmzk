/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <string>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain { namespace contracts {

struct authorizer_ref {
public:
    enum ref_type { owner_t = 0, account_t, group_t, script_t };

private:
    struct script_name_t { name128 name; };
    using owner_type = uint8_t;
    using storage_type = static_variant<owner_type, public_key_type, group_name, script_name_t>;

public:
    authorizer_ref() 
        : storage_(owner_type()) {}

    authorizer_ref(const public_key_type& pkey) 
        : storage_(pkey) {}

public:
    const public_key_type&
    get_account() const {
        return storage_.get<public_key_type>();
    }

    const group_name&
    get_group() const {
        return storage_.get<group_name>();
    }

    const script_name&
    get_script() const {
        return storage_.get<script_name_t>().name;
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

    void
    set_script(const script_name& name) {
        storage_ = script_name_t { .name = name };
    }

    int type() const { return storage_.which(); }

    bool is_account_ref() const { return type() == account_t; }
    bool is_owner_ref() const { return type() == owner_t; }
    bool is_group_ref() const { return type() == group_t; }
    bool is_script_ref() const { return type() == script_t; }

    std::string to_string() const;

private:
    storage_type storage_;

private:
    friend struct fc::reflector<authorizer_ref::script_name_t>;
    friend struct fc::reflector<authorizer_ref>;
};

}}}  // namespac jmzk::chain::contracts

namespace fc {

class variant;
void to_variant(const jmzk::chain::contracts::authorizer_ref& ref, fc::variant& v);
void from_variant(const fc::variant& v, jmzk::chain::contracts::authorizer_ref& ref);

}  // namespace fc

FC_REFLECT(jmzk::chain::contracts::authorizer_ref::script_name_t, (name));
FC_REFLECT(jmzk::chain::contracts::authorizer_ref, (storage_));
