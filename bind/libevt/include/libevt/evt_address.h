/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include "evt.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef evt_data_t evt_public_key_t;
typedef evt_data_t evt_address_t;

int evt_address_from_string(const char* str, evt_address_t** addr /* out */);
int evt_address_to_string(evt_address_t* addr, char** str /* out */);
int evt_address_public_key(evt_public_key_t *pub_key, evt_address_t** addr/* out */);
int evt_address_reserved(evt_address_t** addr/* out */);
int evt_address_generated(const char* prefix, const char* key, uint32_t nonce, evt_address_t** addr/* out */);
int evt_address_get_public_key(evt_address_t* addr, evt_public_key_t **pub_key/* out */);
int evt_address_get_prefix(evt_address_t* addr, char** str/* out */);
int evt_address_get_key(evt_address_t* addr, char** str/* out */);
int evt_address_get_nonce(evt_address_t* addr, uint32_t* nonce/* out */);
int evt_address_get_type(evt_address_t* addr, char** str/* out */);

#ifdef __cplusplus
} // extern "C"
#endif
