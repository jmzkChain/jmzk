/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <libevt/evt.h>
#include <fc/io/raw.hpp>

template <typename T>
evt_data_t*
get_evt_data(const T& val) {
    auto rsz = fc::raw::pack_size<T>(val);
    auto sz  = sizeof(evt_data_t) + rsz;

    auto data = (evt_data_t*)malloc(sz);
    data->sz  = rsz;

    auto ds = fc::datastream<char*>(data->buf, rsz);
    fc::raw::pack(ds, val);

    return data;
}

template <typename T>
int
extract_data(evt_data_t* data, T& val) {
    auto ds = fc::datastream<char*>(data->buf, data->sz);
    try {
        fc::raw::unpack(ds, val);
    }
    catch(...) {
        return EVT_INVALID_BINARY;
    }

    return EVT_OK;
}

inline char*
strdup(const std::string& str) {
    auto s = (char*)malloc(str.size() + 1); // add '\0'
    memcpy(s, str.data(), str.size());
    s[str.size()] = '\0';
    return s;
}
