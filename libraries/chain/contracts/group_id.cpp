/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <eosio/chain/contracts/group_id.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>

namespace eosio { namespace chain { namespace contracts {

group_id
group_id::from_base58(const std::string& base58) {
    char out[sizeof(value_)];
    auto sz = fc::from_base58(base58, out, sizeof(value_));
    FC_ASSERT(sz == sizeof(value_), "Not valid group id");
    auto v = *(__uint128_t*)out;
    return group_id(v);
}

group_id
group_id::from_group_key(const fc::crypto::public_key& pkey) {
    auto sha256 = fc::sha256::hash(pkey);
    auto ripemd160 = fc::ripemd160::hash(sha256);
    auto id = *(__uint128_t*)(ripemd160.data());
    return group_id(id);
}

std::string
group_id::to_base58() const {
    return fc::to_base58((const char*)&value_, sizeof(value_));
}

}}}  // namespac eosio::chain::contracts