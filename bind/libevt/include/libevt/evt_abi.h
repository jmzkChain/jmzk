/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stdint.h>
#include "evt.h"
#include "evt_ecc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef evt_data_t evt_bin_t;
typedef evt_data_t evt_chain_id_t;
typedef evt_data_t evt_block_id_t;

void* evt_abi();
void  evt_free_abi(void* abi);

int evt_abi_json_to_bin(void* evt_abi, const char* action, const char* json, evt_bin_t** bin /* out */);
int evt_abi_bin_to_json(void* evt_abi, const char* action, evt_bin_t* bin, char** json /* out */);
int evt_trx_json_to_digest(void* evt_abi, const char* json, evt_chain_id_t* chain_id, evt_checksum_t** digest /* out */);
int evt_chain_id_from_string(const char* str, evt_chain_id_t** chain_id /* out */);
int evt_block_id_from_string(const char* str, evt_block_id_t** block_id /* out */);
int evt_ref_block_num(evt_block_id_t* block_id, uint16_t* ref_block_num);
int evt_ref_block_prefix(evt_block_id_t* block_id, uint32_t* ref_block_prefix);

#ifdef __cplusplus
} // extern "C"
#endif
