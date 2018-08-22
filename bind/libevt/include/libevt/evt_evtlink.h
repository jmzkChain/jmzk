/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include "evt.h"
#include <stdint.h>
#include <evt/chain/contracts/evt_link.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* evt_link_t;
typedef evt_data_t evt_signature_t;
typedef evt_data_t evt_private_key_t;

evt_link_t evt_link_new();
void evt_link_free(evt_link_t);
int evt_link_tostring(evt_link_t, char**);
int evt_link_parse_from_evtli(const char*, evt_link_t);
int evt_link_get_header(evt_link_t, uint16_t*);
int evt_link_set_header(evt_link_t, uint16_t);
int evt_link_get_segment_int(evt_link_t, uint8_t, uint32_t*);
int evt_link_get_segment_str(evt_link_t, uint8_t, char**);
int evt_link_add_segment_int(evt_link_t, uint8_t, uint32_t);
int evt_link_add_segment_str(evt_link_t, uint8_t, const char*);
int evt_link_get_signatures(evt_link_t, evt_signature_t***, uint32_t*);
int evt_link_sign(evt_link_t, evt_private_key_t*);

#ifdef __cplusplus
} // extern "C"
#endif
