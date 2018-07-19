/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <libevt/evt_address.h>
#include "evt_impl.hpp"

#include <string.h>
#include <evt/chain/name.hpp>
#include <evt/chain/address.hpp>
#include <fc/crypto/public_key.hpp>

using evt::chain;
using evt::chain::address;
using fc::crypto::public_key;

extern "C" {

int
evt_address_from_string(const char* str, evt_address_t** addr /* out */) {
    if (str == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _addr = address(std::string(str));
        *addr = get_evt_data(_addr);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

int
evt_address_to_string(evt_address_t* addr, char** str /* out */) {
    if (addr == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto _addr = address();
    if(extract_data(addr, _addr) != EVT_OK) {
        return EVT_INVALID_ADDRESS;
    }
    try {
        auto out = _addr.to_string();
        *str = strdup(out);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

int
evt_address_public_key(evt_public_key_t *pub_key, evt_address_t** addr/* out */) {
    if (pub_key == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto pkey = public_key();
    if(extract_data(pub_key, pkey) != EVT_OK) {
        return EVT_INVALID_PUBLIC_KEY;
    }
    try {
        auto _addr = address(pkey);
        *addr = get_evt_data(_addr);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

int
evt_address_reserved(evt_address_t** addr/* out */) {
    try {
        auto _addr = address();
        *addr = get_evt_data(_addr);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

int
evt_address_generated(const char* prefix, const char* key, uint32_t nonce, evt_address_t** addr/* out */) {
    try {
        auto _prefix = name(prefix);
        auto _key = name128(key);
        auto _addr = address(_prefix, _key, nonce);
        *addr = get_evt_data(_addr);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

}