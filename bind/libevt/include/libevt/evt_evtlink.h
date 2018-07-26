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

typedef evt_data_t evt_link_t;

int evt_link_parse_from_evtli(const char* str, evt_link_t** link/* out */);
int evt_link_get_header(evt_link_t* link, uint16_t* header/* out */);
int evt_link_set_header(evt_link_t** link/* in&out */, uint16_t header);
int evt_link_get_segment(evt_link_t* link, uint8_t key, uint32_t* intv, char** strv);

#ifdef __cplusplus
} // extern "C"
#endif
