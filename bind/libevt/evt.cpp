#include <libevt/evt.h>
#include <string.h>
#include <stdlib.h>

extern "C" {

int
evt_free(evt_data_t* ptr) {
    if(ptr == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    free(ptr);
    return EVT_OK;
}

int
evt_equals(evt_data_t* rhs, evt_data_t* lhs) {
    if(rhs == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(lhs == nullptr) {
        return EVT_INVALID_ARGUMENT;
    }
    if(rhs->sz != lhs->sz) {
        return EVT_SIZE_NOT_EQUALS;
    }
    if(rhs->buf == lhs->buf) {
        return EVT_OK;
    }
    return memcmp(rhs->buf, lhs->buf, rhs->sz) == 0 ? EVT_OK : EVT_DATA_NOT_EQUALS;
}

}  // extern "C"