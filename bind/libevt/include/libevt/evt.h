/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t  sz;
    char    buf[0];
} evt_data_t;

#define EVT_OK                       0
#define EVT_INTERNAL_ERROR          -1
#define EVT_INVALID_ARGUMENT        -2
#define EVT_INVALID_PRIVATE_KEY     -3
#define EVT_INVALID_PUBLIC_KEY      -4
#define EVT_INVALID_SIGNATURE       -5
#define EVT_INVALID_HASH            -6
#define EVT_SIZE_NOT_EQUALS         -11
#define EVT_DATA_NOT_EQUALS         -12

int evt_free(evt_data_t*);
int evt_equals(evt_data_t* rhs, evt_data_t* lhs);

#ifdef __cplusplus
} // extern "C"
#endif
