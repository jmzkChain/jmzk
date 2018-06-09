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

template <typename T>
size_t
get_type_size() {
    return sizeof(T);
}

template <typename T>
evt_data_t*
get_evt_data(const T& val) {
    auto sz = sizeof(evt_data_t) + get_type_size<T>();
    auto data = (evt_data_t*)malloc(sz);
    data->sz = get_type_size<T>();
    memcpy((char*)data + sizeof(evt_data_t), &val, get_type_size<T>());
    return data;
}

template <typename T>
int
extract_data(evt_data_t* data, T& val) {
    if(data->sz != get_type_size<T>()) {
        return EVT_INVALID_ARGUMENT;
    }
    memcpy(&val, data->buf, get_type_size<T>());
    return EVT_OK;
}

inline char*
strdup(const std::string& str) {
    auto s = (char*)malloc(str.size() + 1); // add '\0'
    memcpy(s, str.data(), str.size());
    s[str.size()] = '\0';
    return s;
}
