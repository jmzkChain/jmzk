/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <libjmzk/jmzk_jmzklink.h>
#include "jmzk_impl.hpp"

#include <string.h>
#include <stddef.h>
#include <jmzk/chain/contracts/jmzk_link.hpp>
#include <fc/crypto/private_key.hpp>

using jmzk::chain::contracts::jmzk_link;
using fc::crypto::private_key;

extern "C" {

jmzk_link_t*
jmzk_link_new() {
    auto linkp = new jmzk_link();
    return (jmzk_link_t*)linkp;
}

void
jmzk_link_free(jmzk_link_t* linkp) {
    delete (jmzk_link*)(linkp);
}

int
jmzk_link_tostring(jmzk_link_t* linkp, char** str) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _str = ((jmzk_link*)(linkp))->to_string();
        *str = strdup(_str);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_parse_from_jmzkli(const char* str, jmzk_link_t* linkp) {
    if (str == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        *((jmzk_link*)(linkp)) = jmzk_link::parse_from_jmzkli(std::string(str));
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_get_header(jmzk_link_t* linkp, uint16_t* header/* out */) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        *header = ((jmzk_link*)(linkp))->get_header();
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_set_header(jmzk_link_t* linkp, uint16_t header) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        ((jmzk_link*)(linkp))->set_header(header);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_get_segment_int(jmzk_link_t* linkp, uint8_t key, uint32_t* intv) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _segment = ((jmzk_link*)(linkp))->get_segment(key);
        if(_segment.intv) {
            *intv = *_segment.intv;
        }
        else {
            return jmzk_INVALID_ARGUMENT;
        }
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_get_segment_str(jmzk_link_t* linkp, uint8_t key, char** strv) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _segment = ((jmzk_link*)(linkp))->get_segment(key);
        if(_segment.strv) {
            *strv = strdup(*_segment.strv);
        }
        else {
            return jmzk_INVALID_ARGUMENT;
        }
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_add_segment_int(jmzk_link_t* linkp, uint8_t key, uint32_t intv) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _segment = jmzk_link::segment(key, intv);
        ((jmzk_link*)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_add_segment_str(jmzk_link_t* linkp, uint8_t key, const char* strv) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _segment = jmzk_link::segment(key, std::string(strv));
        ((jmzk_link*)(linkp))->add_segment(_segment);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_clear_signatures(jmzk_link_t* linkp) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        ((jmzk_link*)(linkp))->clear_signatures();
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_get_signatures(jmzk_link_t* linkp, jmzk_signature_t*** signs, uint32_t* len) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    try {
        auto _signs = ((jmzk_link*)(linkp))->get_signatures();
        auto size   = _signs.size();
        auto psigns = (jmzk_signature_t**)malloc(sizeof(jmzk_signature_t*) * size);

        int i = 0;
        for(auto it = _signs.begin(); it != _signs.end(); it++, i++) {
            psigns[i] = get_jmzk_data(*it);
        }
        
        *signs = psigns;
        *len   = size;
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

int
jmzk_link_sign(jmzk_link_t* linkp, jmzk_private_key_t* priv_key) {
    if (linkp == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    auto pk = private_key();
    if (extract_data(priv_key, pk) != jmzk_OK) {
        return jmzk_INVALID_PRIVATE_KEY;
    }
    try {
        ((jmzk_link*)(linkp))->sign(pk);
    }
    CATCH_AND_RETURN(jmzk_INTERNAL_ERROR)
    return jmzk_OK;
}

}
