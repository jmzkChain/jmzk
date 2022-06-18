/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include "jmzk.h"
#include <stdint.h>
#include <jmzk/chain/contracts/jmzk_link.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef void jmzk_link_t;
typedef jmzk_data_t jmzk_signature_t;
typedef jmzk_data_t jmzk_private_key_t;

jmzk_link_t* jmzk_link_new();
void jmzk_link_free(jmzk_link_t*);
int jmzk_link_tostring(jmzk_link_t*, char**);
int jmzk_link_parse_from_jmzkli(const char*, jmzk_link_t*);
int jmzk_link_get_header(jmzk_link_t*, uint16_t*);
int jmzk_link_set_header(jmzk_link_t*, uint16_t);
int jmzk_link_get_segment_int(jmzk_link_t*, uint8_t, uint32_t*);
int jmzk_link_get_segment_str(jmzk_link_t*, uint8_t, char**);
int jmzk_link_add_segment_int(jmzk_link_t*, uint8_t, uint32_t);
int jmzk_link_add_segment_str(jmzk_link_t*, uint8_t, const char*);
int jmzk_link_get_signatures(jmzk_link_t*, jmzk_signature_t***, uint32_t*);
int jmzk_link_sign(jmzk_link_t*, jmzk_private_key_t*);

#ifdef __cplusplus
} // extern "C"
#endif
