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

int evt_link_new(LinkPtr*);
int evt_link_free(LinkPtr*);
int evt_link_tostring(LinkPtr, char**);
int evt_link_parse_from_evtli(const char*, LinkPtr);
int evt_link_get_header(LinkPtr, uint16_t*);
int evt_link_set_header(LinkPtr, uint16_t);
int evt_link_get_segment(LinkPtr, uint8_t, uint32_t*, char**);
int evt_link_add_segment_int(LinkPtr, uint8_t, uint32_t);
int evt_link_add_segment_str(LinkPtr, uint8_t, const char*);
int evt_link_get_signatures(LinkPtr, evt_signature_t***, uint32_t*);
int evt_link_sign(LinkPtr, evt_private_key_t*);

#ifdef __cplusplus
} // extern "C"
#endif
