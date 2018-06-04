/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include "libevt/evt_abi.h"
#include "evt_impl.hpp"
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/types.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

using evt::chain::contracts::abi_serializer;
using evt::chain::contracts::abi_def;
using evt::chain::bytes;

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
    val.reserve(data->sz);
    memcpy(&val[0], data->buf, data->sz);
    return EVT_OK;
}

extern "C" {

void*
evt_api() {
    auto api = new abi_def(evt::chain::contracts::evt_contract_abi());
    return (void*)api;
}

void
free_evt_api(void* api) {
    delete (abi_def*)api;
}

int
evt_abi_json_to_bin(void* evt_api, const char* action, const char* json, evt_data_t** bin /* out */) {
    auto abi = abi_serializer(*(abi_def*)evt_api);
    auto var = fc::json::from_string(json);
    auto action_type = abi.get_action_type(action);
    if(action_type.empty()) {
        return EVT_INVALID_ACTION;
    }
    try {
        auto b = abi.variant_to_binary(action_type, var);
        auto data = get_evt_data(b);
        *bin = data;
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

int
evt_abi_bin_to_json(void* evt_api, const char* action, evt_data_t* bin, char** json /* out */) {
    auto abi = abi_serializer(*(abi_def*)evt_api);
    auto action_type = abi.get_action_type(action);
    if(action_type.empty()) {
        return EVT_INVALID_ACTION;
    }
    try {
        bytes b;
        if(extract_data(bin, b) != EVT_OK) {
            return EVT_INVALID_BINARY;
        }
        auto var = abi.binary_to_variant(action_type, b);
        auto str = fc::json::to_string(var);
        *json = strdup(str);
    }
    catch(...) {
        return EVT_INTERNAL_ERROR;
    }
    return EVT_OK;
}

}  // extern "C"