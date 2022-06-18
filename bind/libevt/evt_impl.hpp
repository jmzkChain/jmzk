/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <libjmzk/jmzk.h>
#include <fc/io/raw.hpp>

#define CATCH_AND_RETURN(err)         \
    catch(fc::exception& e) {         \
        jmzk_set_last_error(e.code()); \
        return err;                   \
    }                                 \
    catch(...) {                      \
        jmzk_set_last_error(-1);       \
        return err;                   \
    }

template <typename T>
jmzk_data_t*
get_jmzk_data(const T& val) {
    auto rsz = fc::raw::pack_size<T>(val);
    auto sz  = sizeof(jmzk_data_t) + rsz;

    auto data = (jmzk_data_t*)malloc(sz);
    data->sz  = rsz;

    auto ds = fc::datastream<char*>(data->buf, rsz);
    fc::raw::pack(ds, val);

    return data;
}

template <typename T>
int
extract_data(jmzk_data_t* data, T& val) {
    auto ds = fc::datastream<char*>(data->buf, data->sz);
    try {
        fc::raw::unpack(ds, val);
    }
    CATCH_AND_RETURN(jmzk_INVALID_BINARY)

    return jmzk_OK;
}

inline char*
strdup(const std::string& str) {
    auto s = (char*)malloc(str.size() + 1); // add '\0'
    memcpy(s, str.data(), str.size());
    s[str.size()] = '\0';
    return s;
}
