/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include "jmzk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef jmzk_data_t jmzk_public_key_t;
typedef jmzk_data_t jmzk_private_key_t;
typedef jmzk_data_t jmzk_signature_t;
typedef jmzk_data_t jmzk_checksum_t;

int jmzk_generate_new_pair(jmzk_public_key_t** pub_key /* out */, jmzk_private_key_t** priv_key /* out */);
int jmzk_get_public_key(jmzk_private_key_t* priv_key, jmzk_public_key_t** pub_key /* out */);
int jmzk_sign_hash(jmzk_private_key_t* priv_key, jmzk_checksum_t* hash, jmzk_signature_t** sign /* out */);
int jmzk_recover(jmzk_signature_t* sign, jmzk_checksum_t* hash, jmzk_public_key_t** pub_key /* out */);
int jmzk_hash(const char* buf, size_t sz, jmzk_checksum_t** hash /* out */);

int jmzk_public_key_string(jmzk_public_key_t* pub_key, char** str /* out */);
int jmzk_private_key_string(jmzk_private_key_t* priv_key, char** str /* out */);
int jmzk_signature_string(jmzk_signature_t* sign, char** str /* out */);
int jmzk_checksum_string(jmzk_checksum_t* hash, char** str /* out */);

int jmzk_public_key_from_string(const char* str, jmzk_public_key_t** pub_key /* out */);
int jmzk_private_key_from_string(const char* str, jmzk_private_key_t** priv_key /* out */);
int jmzk_signature_from_string(const char* str, jmzk_signature_t** sign /* out */);
int jmzk_checksum_from_string(const char* str, jmzk_checksum_t** hash /* out */);

#ifdef __cplusplus
} // extern "C"
#endif
