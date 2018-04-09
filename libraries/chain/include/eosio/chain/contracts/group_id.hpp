/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
#include <fc/crypto/public_key.hpp>
#include <fc/reflect/reflect.hpp>

namespace eosio { namespace chain { namespace contracts {

struct group_id {
public:
    group_id() = default;
    group_id(__uint128_t v) : value_(v) {}

public:
    static group_id from_base58(const std::string& base58);
    static group_id from_group_key(const fc::crypto::public_key& pkey);

public:
    std::string to_base58() const;
    bool empty() const { return value_ == 0; }

public:
    friend std::ostream& operator << ( std::ostream& out, const group_id& n ) {
        return out << n.to_base58();
    }

    friend bool operator < ( const group_id& a, const group_id& b ) { return a.value_ < b.value_; }
    friend bool operator <= ( const group_id& a, const group_id& b ) { return a.value_ <= b.value_; }
    friend bool operator > ( const group_id& a, const group_id& b ) { return a.value_ > b.value_; }
    friend bool operator >=( const group_id& a, const group_id& b ) { return a.value_ >= b.value_; }
    friend bool operator == ( const group_id& a, const group_id& b ) { return a.value_ == b.value_; }

    friend bool operator == ( const group_id& a, uint64_t b ) { return a.value_ == b; }
    friend bool operator != ( const group_id& a, uint64_t b ) { return a.value_ != b; }

    friend bool operator != ( const group_id& a, const group_id& b ) { return a.value_ != b.value_; }

    operator bool() const { return value_; }
    operator __uint128_t() const { return value_; };

public:
    __uint128_t value_;
};

}}}  // namespac eosio::chain::contracts

namespace fc {

class variant;
void to_variant(const eosio::chain::contracts::group_id& gid, fc::variant& v);
void from_variant(const fc::variant& v, eosio::chain::contracts::group_id& gid);

}  // namespace fc

FC_REFLECT(eosio::chain::contracts::group_id, (value_))