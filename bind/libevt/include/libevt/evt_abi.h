/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <stdint.h>
#include "jmzk.h"
#include "jmzk_ecc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef jmzk_data_t jmzk_bin_t;
typedef jmzk_data_t jmzk_chain_id_t;
typedef jmzk_data_t jmzk_block_id_t;

void* jmzk_abi();
void  jmzk_free_abi(void* abi);

int jmzk_abi_json_to_bin(void* jmzk_abi, const char* action, const char* json, jmzk_bin_t** bin /* out */);
int jmzk_abi_bin_to_json(void* jmzk_abi, const char* action, jmzk_bin_t* bin, char** json /* out */);
int jmzk_trx_json_to_digest(void* jmzk_abi, const char* json, jmzk_chain_id_t* chain_id, jmzk_checksum_t** digest /* out */);
int jmzk_chain_id_from_string(const char* str, jmzk_chain_id_t** chain_id /* out */);
int jmzk_block_id_from_string(const char* str, jmzk_block_id_t** block_id /* out */);
int jmzk_ref_block_num(jmzk_block_id_t* block_id, uint16_t* ref_block_num);
int jmzk_ref_block_prefix(jmzk_block_id_t* block_id, uint32_t* ref_block_prefix);

#ifdef __cplusplus
} // extern "C"
#endif
