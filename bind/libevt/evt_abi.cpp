/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include "libjmzk/jmzk_abi.h"
#include "jmzk_impl.hpp"

#include <fc/crypto/sha256.hpp>
#include <fc/bitutil.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <jmzk/chain/execution_context_mock.hpp>
#include <jmzk/chain/contracts/jmzk_contract_abi.hpp>
#include <jmzk/chain/contracts/abi_serializer.hpp>
#include <jmzk/chain/types.hpp>

using fc::sha256;
using jmzk::chain::bytes;
using jmzk::chain::chain_id_type;
using jmzk::chain::jmzk_execution_context_mock;
using jmzk::chain::transaction;
using jmzk::chain::contracts::abi_serializer;
using jmzk::chain::contracts::abi_def;

template <> 
jmzk_data_t* 
get_jmzk_data<bytes>(const bytes& b) { 
    auto sz = sizeof(jmzk_data_t) + b.size(); 
    auto data = (jmzk_data_t*)malloc(sz); 
    data->sz = b.size(); 
    memcpy((char*)data + sizeof(jmzk_data_t), b.data(), b.size()); 
    return data; 
} 
 
template <> 
int 
extract_data<bytes>(jmzk_data_t* data, bytes& val) { 
    if(data->sz == 0) { 
        return jmzk_INVALID_ARGUMENT; 
    } 
    val.resize(data->sz); 
    memcpy(&val[0], data->buf, data->sz); 
    return jmzk_OK; 
} 

struct abi_context {
    std::unique_ptr<abi_serializer>             abi;
    std::unique_ptr<jmzk_execution_context_mock> exec_ctx;
};

extern "C" {

void*
jmzk_abi() {
    auto abic      = new abi_context();
    abic->abi      = std::make_unique<abi_serializer>(jmzk::chain::contracts::jmzk_contract_abi(), std::chrono::hours(1));
    abic->exec_ctx = std::make_unique<jmzk_execution_context_mock>();

    return (void*)abic;
}

void
jmzk_free_abi(void* abi) {
    delete (abi_context*)abi;
}

int
jmzk_abi_json_to_bin(void* jmzk_abi, const char* action, const char* json, jmzk_bin_t** bin /* out */) {
    if(jmzk_abi == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(action == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(bin == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto& abic = *(abi_context*)jmzk_abi;
    auto  var  = fc::variant();
    try {
        var = fc::json::from_string(json);
        if(!var.is_object()) {
            return jmzk_INVALID_JSON;
        }
    }
    CATCH_AND_RETURN(jmzk_INVALID_JSON)

    auto type = abic.exec_ctx->get_acttype_name(action);
    if(type.empty()) {
        return jmzk_INVALID_ACTION;
    }
    try {
        auto b = abic.abi->variant_to_binary(type, var, *abic.exec_ctx);
        if(b.empty()) {
            return jmzk_INVALID_JSON;
        }
        auto data = get_jmzk_data(b);
        *bin = data;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    
    return jmzk_OK;
}

int
jmzk_abi_bin_to_json(void* jmzk_abi, const char* action, jmzk_bin_t* bin, char** json /* out */) {
    if(jmzk_abi == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(action == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(bin == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto& abic = *(abi_context*)jmzk_abi;
    auto  type = abic.exec_ctx->get_acttype_name(action);
    if(type.empty()) {
        return jmzk_INVALID_ACTION;
    }
    try {
        bytes b;
        if(extract_data(bin, b) != jmzk_OK) {
            return jmzk_INVALID_BINARY;
        }
        auto var = abic.abi->binary_to_variant(type, b, *abic.exec_ctx);
        auto str = fc::json::to_string(var);
        *json = strdup(str);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_trx_json_to_digest(void* jmzk_abi, const char* json,  jmzk_chain_id_t* chain_id, jmzk_checksum_t** digest /* out */) {
    if(jmzk_abi == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(json == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(digest == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(chain_id, idhash) != jmzk_OK) {
        return jmzk_INVALID_HASH;
    }

    auto& abic = *(abi_context*)jmzk_abi;
    auto  trx  = transaction();
    try {
        auto var = fc::json::from_string(json);
        abic.abi->from_variant(var, trx, *abic.exec_ctx);

        auto d  = trx.sig_digest(chain_id_type(idhash));
        *digest = get_jmzk_data(d);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)

    return jmzk_OK;
}

int
jmzk_chain_id_from_string(const char* str, jmzk_chain_id_t** chain_id /* out */) {
    return jmzk_checksum_from_string(str, chain_id);
}

int
jmzk_block_id_from_string(const char* str, jmzk_block_id_t** block_id /* out */) {
    return jmzk_checksum_from_string(str, block_id);
}

int
jmzk_ref_block_num(jmzk_block_id_t* block_id, uint16_t* ref_block_num) {
    if(block_id == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(block_id, idhash) != jmzk_OK) {
        return jmzk_INVALID_HASH;
    }
    *ref_block_num = (uint16_t)fc::endian_reverse_u32(idhash._hash[0]);
    return jmzk_OK;
}

int
jmzk_ref_block_prefix(jmzk_block_id_t* block_id, uint32_t* ref_block_prefix) {
    if(block_id == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    sha256 idhash;
    if(extract_data(block_id, idhash) != jmzk_OK) {
        return jmzk_INVALID_HASH;
    }
    *ref_block_prefix = (uint32_t)idhash._hash[1];
    return jmzk_OK;
}

}  // extern "C"