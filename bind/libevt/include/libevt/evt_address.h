/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include "jmzk.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef jmzk_data_t jmzk_public_key_t;
typedef jmzk_data_t jmzk_address_t;

int jmzk_address_from_string(const char* str, jmzk_address_t** addr /* out */);
int jmzk_address_to_string(jmzk_address_t* addr, char** str /* out */);
int jmzk_address_public_key(jmzk_public_key_t *pub_key, jmzk_address_t** addr/* out */);
int jmzk_address_reserved(jmzk_address_t** addr/* out */);
int jmzk_address_generated(const char* prefix, const char* key, uint32_t nonce, jmzk_address_t** addr/* out */);
int jmzk_address_get_public_key(jmzk_address_t* addr, jmzk_public_key_t **pub_key/* out */);
int jmzk_address_get_prefix(jmzk_address_t* addr, char** str/* out */);
int jmzk_address_get_key(jmzk_address_t* addr, char** str/* out */);
int jmzk_address_get_nonce(jmzk_address_t* addr, uint32_t* nonce/* out */);
int jmzk_address_get_type(jmzk_address_t* addr, char** str/* out */);

#ifdef __cplusplus
} // extern "C"
#endif
