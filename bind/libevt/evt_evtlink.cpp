/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <libevt/evt_evtlink.h>
#include "evt_impl.hpp"

#include <string.h>
#include <stddef.h>
#include <evt/chain/contracts/evt_link.hpp>

using evt::chain::contracts::evt_link;

extern "C" {

int
evt_link_parse_from_evtli(const char* str, evt_link_t** link/* out */) {
    if (str == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    try {
        auto _link = evt_link::parse_from_evtli(std::string(str));
        *link = get_evt_data(_link);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)

    return EVT_OK;
}

int
evt_link_get_header(evt_link_t* link, uint16_t* header/* out */) {
    if (link == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto _link = evt_link();
    if (extract_data(link, _link) != EVT_OK) {
        return EVT_INVALID_LINK;
    }

    try {
        auto _header = _link.get_header();
        *header = _header;
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)

    return EVT_OK;
}

int
evt_link_set_header(evt_link_t** link/* in&out */, uint16_t header) {
    if (link == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto _link = evt_link();
    if (extract_data(*link, _link) != EVT_OK) {
        return EVT_INVALID_LINK;
    }

    try {
        _link.set_header(header);
        *link = get_evt_data(_link);
    }
    CATCH_AND_RETURN(EVT_INTERNAL_ERROR)

    return EVT_OK;
}

int
evt_link_get_segment(evt_link_t* link, uint8_t key, uint32_t* intv /* out */, char** strv /* out */) {
    if (link == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    auto _link = evt_link();
    if (extract_data(link, _link) != EVT_OK) {
        return EVT_INVALID_LINK;
    }
    
    try {
        auto _segment = _link.get_segment(key);
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

}
