/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <libevt/evt_evtlink.h>
#include "evt_impl.hpp"

#include <string.h>
#include <stddef.h>
#include <evt/chain/contracts/evt_link.hpp>
#include <fc/crypto/private_key.hpp>

using evt::chain::contracts::evt_link;
using fc::crypto::private_key;

extern "C" {

int
evt_link_new(void** linkp) {
    try {
        *linkp = new evt_link();
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_free(void** linkp) {
    delete (LinkPtr)(*linkp);
    *linkp = nullptr;
    return EVT_OK;
}

int
evt_link_tostring(void* linkp, char** str) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _str = ((LinkPtr)(linkp))->to_string();
        *str = strdup(_str);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_parse_from_evtli(const char* str, void* linkp) {
    if (str == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        *((LinkPtr)(linkp)) = evt_link::parse_from_evtli(std::string(str));
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_header(void* linkp, uint16_t* header/* out */) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        *header = ((LinkPtr)(linkp))->get_header();
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_set_header(void* linkp, uint16_t header) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        ((LinkPtr)(linkp))->set_header(header);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_segment(void* linkp, uint8_t key, uint32_t* intv, char** strv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = ((LinkPtr)(linkp))->get_segment(key);
        if (_segment.intv.valid()) {
            *intv = *_segment.intv;
        }
        else {
            *strv = strdup(*_segment.strv);
        }
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_add_segment_int(void* linkp, uint8_t key, uint32_t intv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = evt_link::segment(key, intv);
        ((LinkPtr)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_add_segment_str(void* linkp, uint8_t key, const char* strv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = evt_link::segment(key, std::string(strv));
        ((LinkPtr)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_clear_signatures(void* linkp) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        ((LinkPtr)(linkp))->clear_signatures();
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_signatures(void* linkp, evt_signature_t*** signs, uint32_t* len) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _signs = ((LinkPtr)(linkp))->get_signatures();
        int size = _signs.size();
        *signs = new evt_signature_t*[size];
        int i = 0;
        for(auto it = _signs.begin(); it != _signs.end(); it++, i++) {
            *(signs[i]) = get_evt_data(*it);
        }
        *len = size;
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_sign(void* linkp, evt_private_key_t* priv_key) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto pk = private_key();
    if (extract_data(priv_key, pk) != EVT_OK) {
        return EVT_INVALID_PRIVATE_KEY;
    }
    try {
        ((LinkPtr)(linkp))->sign(pk);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

}
