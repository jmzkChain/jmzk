/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include "evt.h"

#ifdef __cplusplus
extern "C" {
#endif

void* evt_api();
void free_evt_api(void* api);

int evt_abi_json_to_bin(void* evt_api, const char* action, const char* json, evt_data_t** bin /* out */);
int evt_abi_bin_to_json(void* evt_api, const char* action, evt_data_t* bin, char** json /* out */);

#ifdef __cplusplus
} // extern "C"
#endif
