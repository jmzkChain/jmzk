/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include "libevt/evt_abi.h"
#include "evt_impl.hpp"
#include <evt/chain/contracts/evt_contract_abi.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/types.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/bitutil.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>

using fc::sha256;
using evt::chain::bytes;
using evt::chain::chain_id_type;
using evt::chain::transaction;
using evt::chain::contracts::abi_serializer;
using evt::chain::contracts::abi_def;

template <> 
evt_data_t* 
get_evt_data<bytes>(const bytes& b) { 
    auto sz = sizeof(evt_data_t) + b.size(); 
    auto data = (evt_data_t*)malloc(sz); 
    data->sz = b.size(); 
    memcpy((char*)data + sizeof(evt_data_t), b.data(), b.size()); 
    return data; 
} 
 
template <> 
int 
extract_data<bytes>(evt_data_t* data, bytes& val) { 
    if(data->sz == 0) { 
        return EVT_INVALID_ARGUMENT; 
    } 
    val.resize(data->sz); 
    memcpy(&val[0], data->buf, data->sz); 
    return EVT_OK; 
} 

extern "C" {

void*
evt_abi() {
    auto abi = new abi_serializer(evt::chain::contracts::evt_contract_abi(), std::chrono::hours(1));
    return (void*)abi;
}

void
evt_free_abi(void* abi) {
    delete (abi_serializer*)abi;
}

int
evt_abi_json_to_bin(void* evt_abi, const char* action, const char* json, evt_bin_t** bin /* out */) {
    if(evt_abi == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(action == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(bin == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto& abi = *(abi_serializer*)evt_abi;
    auto  var = fc::variant();
    try {
        var = fc::json::from_string(json);
        if(!var.is_object()) {
            return EVT_INVALID_JSON;
        }
    }
    CATCH_AND_RETURN(EVT_INVALID_JSON)

    auto type = abi.get_action_type(action);
    if(type.empty()) {
        return EVT_INVALID_ACTION;
    }
    try {
        auto b = abi.variant_to_binary(type, var);
        if(b.empty()) {
            return EVT_INVALID_JSON;
        }
        auto data = get_evt_data(b);
        *bin = data;
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    
    return EVT_OK;
}

int
evt_abi_bin_to_json(void* evt_abi, const char* action, evt_bin_t* bin, char** json /* out */) {
    if(evt_abi == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(action == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(bin == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto& abi  = *(abi_serializer*)evt_abi;
    auto  type = abi.get_action_type(action);
    if(type.empty()) {
        return EVT_INVALID_ACTION;
    }
    try {
        bytes b;
        if(extract_data(bin, b) != EVT_OK) {
            return EVT_INVALID_BINARY;
        }
        auto var = abi.binary_to_variant(type, b);
        auto str = fc::json::to_string(var);
        *json = strdup(str);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)

    return EVT_OK;
}

int
evt_trx_json_to_digest(void* evt_abi, const char* json,  evt_chain_id_t* chain_id, evt_checksum_t** digest /* out */) {
    if(evt_abi == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(digest == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(chain_id, idhash) != EVT_OK) {
        return EVT_INVALID_HASH;
    }

    auto& abi = *(abi_serializer*)evt_abi;
    auto  trx = transaction();
    try {
        auto var = fc::json::from_string(json);
        abi.from_variant(var, trx);

        auto d  = trx.sig_digest(chain_id_type(idhash));
        *digest = get_evt_data(d);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)

    return EVT_OK;
}

int
evt_chain_id_from_string(const char* str, evt_chain_id_t** chain_id /* out */) {
    return evt_checksum_from_string(str, chain_id);
}

int
evt_block_id_from_string(const char* str, evt_block_id_t** block_id /* out */) {
    return evt_checksum_from_string(str, block_id);
}

int
evt_ref_block_num(evt_block_id_t* block_id, uint16_t* ref_block_num) {
    if(block_id == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(block_id, idhash) != EVT_OK) {
        return EVT_INVALID_HASH;
    }
    *ref_block_num = (uint16_t)fc::endian_reverse_u32(idhash._hash[0]);
    return EVT_OK;
}

int
evt_ref_block_prefix(evt_block_id_t* block_id, uint32_t* ref_block_prefix) {
    if(block_id == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(block_id, idhash) != EVT_OK) {
        return EVT_INVALID_HASH;
    }
    *ref_block_prefix = (uint32_t)idhash._hash[1];
    return EVT_OK;
}

}  // extern "C"