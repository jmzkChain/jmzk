#include <libevt/evt.h>
#include <string.h>
#include <stdlib.h>
#include <evt/chain/version.hpp>
#include <evt/chain/contracts/evt_contract_abi.hpp>

extern "C" {

int
evt_free(void* ptr) {
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

int
evt_version() {
    auto ver = evt::chain::contracts::evt_contract_abi_version();
    return ver.get_version();
}

thread_local int last_error_code = 0;

int
evt_set_last_error(int code) {
    last_error_code = code;
    return code;
}

int
evt_last_error() {
    return last_error_code;
}

}  // extern "C"