/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include "evt.h"
#include <stdint.h>
#include <evt/chain/contracts/evt_link.hpp>
using evt::chain::contracts::evt_link;

#ifdef __cplusplus
extern "C" {
#endif

typedef evt_link* LinkPtr;
typedef evt_data_t evt_signature_t;
typedef evt_data_t evt_private_key_t;

int evt_link_new(void**);
int evt_link_free(void**);
int evt_link_tostring(void*, char**);
int evt_link_parse_from_evtli(const char*, void*);
int evt_link_get_header(void*, uint16_t*);
int evt_link_set_header(void*, uint16_t);
int evt_link_get_segment(void*, uint8_t, uint32_t*, char**);
int evt_link_add_segment_int(void*, uint8_t, uint32_t);
int evt_link_add_segment_str(void*, uint8_t, const char*);
int evt_link_get_signatures(void*, evt_signature_t***, uint32_t*);
int evt_link_sign(void*, evt_private_key_t*);

#ifdef __cplusplus
} // extern "C"
#endif
