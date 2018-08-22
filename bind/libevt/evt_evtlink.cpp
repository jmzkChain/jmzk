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

evt_link_t
evt_link_new() {
    auto linkp = new evt_link();
    return (evt_link_t)linkp;
}

void
evt_link_free(evt_link_t linkp) {
    delete (evt_link*)(linkp);
}

int
evt_link_tostring(evt_link_t linkp, char** str) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _str = ((evt_link*)(linkp))->to_string();
        *str = strdup(_str);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_parse_from_evtli(const char* str, evt_link_t linkp) {
    if (str == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        *((evt_link*)(linkp)) = evt_link::parse_from_evtli(std::string(str));
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_header(evt_link_t linkp, uint16_t* header/* out */) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        *header = ((evt_link*)(linkp))->get_header();
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_set_header(evt_link_t linkp, uint16_t header) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        ((evt_link*)(linkp))->set_header(header);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_segment_int(evt_link_t linkp, uint8_t key, uint32_t* intv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = ((evt_link*)(linkp))->get_segment(key);
        *intv = *_segment.intv;
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_segment_str(evt_link_t linkp, uint8_t key, char** strv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = ((evt_link*)(linkp))->get_segment(key);
        *strv = strdup(*_segment.strv);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_add_segment_int(evt_link_t linkp, uint8_t key, uint32_t intv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = evt_link::segment(key, intv);
        ((evt_link*)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_add_segment_str(evt_link_t linkp, uint8_t key, const char* strv) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _segment = evt_link::segment(key, std::string(strv));
        ((evt_link*)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_clear_signatures(evt_link_t linkp) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        ((evt_link*)(linkp))->clear_signatures();
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

int
evt_link_get_signatures(evt_link_t linkp, evt_signature_t*** signs, uint32_t* len) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _signs = ((evt_link*)(linkp))->get_signatures();
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
evt_link_sign(evt_link_t linkp, evt_private_key_t* priv_key) {
    if (linkp == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto pk = private_key();
    if (extract_data(priv_key, pk) != EVT_OK) {
        return EVT_INVALID_PRIVATE_KEY;
    }
    try {
        ((evt_link*)(linkp))->sign(pk);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)
    return EVT_OK;
}

}
