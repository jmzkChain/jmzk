/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t  sz;
    char    buf[0];
} jmzk_data_t;

#define jmzk_OK                       0
#define jmzk_INTERNAL_ERROR          -1
#define jmzk_INVALID_ARGUMENT        -2
#define jmzk_INVALID_PRIVATE_KEY     -3
#define jmzk_INVALID_PUBLIC_KEY      -4
#define jmzk_INVALID_SIGNATURE       -5
#define jmzk_INVALID_HASH            -6
#define jmzk_INVALID_ACTION          -7
#define jmzk_INVALID_BINARY          -8
#define jmzk_INVALID_JSON            -9
#define jmzk_INVALID_ADDRESS         -10
#define jmzk_SIZE_NOT_EQUALS         -11
#define jmzk_DATA_NOT_EQUALS         -12
#define jmzk_INVALID_LINK            -13

int jmzk_free(void*);
int jmzk_equals(jmzk_data_t* rhs, jmzk_data_t* lhs);
int jmzk_version();
int jmzk_set_last_error(int code);
int jmzk_last_error();

#ifdef __cplusplus
} // extern "C"
#endif
