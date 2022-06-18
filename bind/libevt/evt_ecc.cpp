/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <libjmzk/jmzk_ecc.h>
#include "jmzk_impl.hpp"

#include <string.h>
#include <limits>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/public_key.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/crypto/sha256.hpp>

using fc::sha256;
using fc::crypto::private_key;
using fc::crypto::public_key;
using fc::crypto::signature;

extern "C" {

int
jmzk_generate_new_pair(jmzk_public_key_t** pub_key /* out */, jmzk_private_key_t** priv_key /* out */) {
    if(pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(priv_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    try {
        auto pk  = private_key::generate();
        auto pub = pk.get_public_key();

        *pub_key  = get_jmzk_data(pub);
        *priv_key = get_jmzk_data(pk);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_get_public_key(jmzk_private_key_t* priv_key, jmzk_public_key_t** pub_key /* out */) {
    if(priv_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pk = private_key();
    if(extract_data(priv_key, pk) != jmzk_OK) {
        return jmzk_INVALID_PRIVATE_KEY;
    }
    try {
        auto pub = pk.get_public_key();
        *pub_key = get_jmzk_data(pub);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_sign_hash(jmzk_private_key_t* priv_key, jmzk_checksum_t* hash, jmzk_signature_t** sign /* out */) {
    if(priv_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(hash == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(sign == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pk = private_key();
    if(extract_data(priv_key, pk) != jmzk_OK) {
        return jmzk_INVALID_PRIVATE_KEY;
    }
    auto h = sha256();
    if(extract_data(hash, h) != jmzk_OK) {
        return jmzk_INVALID_SIGNATURE;
    }
    try {
        auto sig = pk.sign(h);
        *sign = get_jmzk_data(sig);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_recover(jmzk_signature_t* sign, jmzk_checksum_t* hash, jmzk_public_key_t** pub_key /* out */) {
    if(sign == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(hash == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto sig = signature();
    auto h = sha256();
    if(extract_data(sign, sig) != jmzk_OK) {
        return jmzk_INVALID_SIGNATURE;
    }
    if(extract_data(hash, h) != jmzk_OK) {
        return jmzk_INVALID_HASH;
    }
    try {
        auto pkey = public_key(sig, h);
        *pub_key = get_jmzk_data(pkey);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_hash(const char* buf, size_t sz, jmzk_checksum_t** hash /* out */) {
    if(buf == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(sz == 0 || sz >= std::numeric_limits<uint32_t>::max()) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(hash == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto h = sha256::hash(buf, sz);
        *hash = get_jmzk_data(h);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_public_key_string(jmzk_public_key_t* pub_key, char** str /* out */) {
    if(pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pkey = public_key();
    if(extract_data(pub_key, pkey) != jmzk_OK) {
        return jmzk_INVALID_PUBLIC_KEY;
    }
    auto pkey_str = (std::string)pkey;
    *str = strdup(pkey_str);
    return jmzk_OK;
}

int
jmzk_private_key_string(jmzk_private_key_t* priv_key, char** str /* out */) {
    if(priv_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pkey = private_key();
    if(extract_data(priv_key, pkey) != jmzk_OK) {
        return jmzk_INVALID_PRIVATE_KEY;
    }
    auto pkey_str = (std::string)pkey;
    *str = strdup(pkey_str);
    return jmzk_OK;
}

int
jmzk_signature_string(jmzk_signature_t* sign, char** str /* out */) {
    if(sign == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto sig = signature();
    if(extract_data(sign, sig) != jmzk_OK) {
        return jmzk_INVALID_SIGNATURE;
    }
    auto sign_str = (std::string)sig;
    *str = strdup(sign_str);
    return jmzk_OK;
}

int
jmzk_checksum_string(jmzk_checksum_t* hash, char** str /* out */) {
    if(hash == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto h = sha256();
    if(extract_data(hash, h) != jmzk_OK) {
        return jmzk_INVALID_HASH;
    }
    auto hash_str = (std::string)h;
    *str = strdup(hash_str);
    return jmzk_OK;
}

int
jmzk_public_key_from_string(const char* str, jmzk_public_key_t** pub_key /* out */) {
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto pkey = public_key(std::string(str));
        auto data = get_jmzk_data(pkey);
        *pub_key = data;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_private_key_from_string(const char* str, jmzk_private_key_t** priv_key /* out */) {
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(priv_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto pkey = private_key(std::string(str));
        auto data = get_jmzk_data(pkey);
        *priv_key = data;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_signature_from_string(const char* str, jmzk_signature_t** sign /* out */) {
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(sign == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto sig = signature(std::string(str));
        auto data = get_jmzk_data(sig);
        *sign = data;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_checksum_from_string(const char* str, jmzk_checksum_t** hash /* out */) {
    if(str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(hash == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto h = sha256(std::string(str));
        auto data = get_jmzk_data(h);
        *hash = data;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

} // extern "C"