/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <libjmzk/jmzk_address.h>
#include "jmzk_impl.hpp"

#include <string.h>
#include <stddef.h>
#include <jmzk/chain/name.hpp>
#include <jmzk/chain/address.hpp>
#include <fc/crypto/public_key.hpp>

using jmzk::chain::name;
using jmzk::chain::name128;
using jmzk::chain::address;
using fc::crypto::public_key;

extern "C" {

int
jmzk_address_from_string(const char* str, jmzk_address_t** addr /* out */) {
    if (str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _addr = address(std::string(str));
        *addr = get_jmzk_data(_addr);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    
    return jmzk_OK;
}

int
jmzk_address_to_string(jmzk_address_t* addr, char** str /* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    
    auto _addr = address();
    if(extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }
    try {
        auto out = _addr.to_string();
        *str = strdup(out);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_public_key(jmzk_public_key_t *pub_key, jmzk_address_t** addr/* out */) {
    if (pub_key == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pkey = public_key();
    if(extract_data(pub_key, pkey) != jmzk_OK) {
        return jmzk_INVALID_PUBLIC_KEY;
    }
    try {
        auto _addr = address(pkey);
        *addr = get_jmzk_data(_addr);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_reserved(jmzk_address_t** addr/* out */) {
    try {
        auto _addr = address();
        *addr = get_jmzk_data(_addr);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_generated(const char* prefix, const char* key, uint32_t nonce, jmzk_address_t** addr/* out */) {
    try {
        auto _prefix = name(prefix);
        auto _key = name128(key);
        auto _addr = address(_prefix, _key, nonce);
        *addr = get_jmzk_data(_addr);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_get_public_key(jmzk_address_t* addr, jmzk_public_key_t **pub_key/* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    auto _addr = address();
    if(extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }

    try {
        auto pkey = _addr.get_public_key();
        *pub_key = get_jmzk_data(pkey);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_get_prefix(jmzk_address_t* addr, char** str/* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    auto _addr = address();
    if(extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }

    try {
        auto prefix = _addr.get_prefix();
        *str = strdup(prefix.to_string());
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_get_key(jmzk_address_t* addr, char** str/* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    auto _addr = address();
    if(extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }

    try {
        auto key = _addr.get_key();
        *str = strdup(key.to_string());
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_get_nonce(jmzk_address_t* addr, uint32_t* nonce/* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    auto _addr = address();
    if(extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }

    try {
        auto out = _addr.get_nonce();
        *nonce = out;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_address_get_type(jmzk_address_t* addr, char** str/* out */) {
    if (addr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }

    auto _addr = address();
    if (extract_data(addr, _addr) != jmzk_OK) {
        return jmzk_INVALID_ADDRESS;
    }

    try {
        if (_addr.is_reserved()) {
            *str = strdup("reserved");
        }
        else if (_addr.is_public_key()) {
            *str = strdup("public_key");
        }
        else {
            *str = strdup("generated");
        }
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

}
