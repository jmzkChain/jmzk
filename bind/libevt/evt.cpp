#include <libjmzk/jmzk.h>
#include <string.h>
#include <stdlib.h>
#include <jmzk/chain/version.hpp>
#include <jmzk/chain/contracts/jmzk_contract_abi.hpp>

extern "C" {

int
jmzk_free(void* ptr) {
    if(ptr == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    free(ptr);
    return jmzk_OK;
}

int
jmzk_equals(jmzk_data_t* rhs, jmzk_data_t* lhs) {
    if(rhs == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(lhs == nullptr) {
        return jmzk_INVALID_ARGUMENT;
    }
    if(rhs->sz != lhs->sz) {
        return jmzk_SIZE_NOT_EQUALS;
    }
    if(rhs->buf == lhs->buf) {
        return jmzk_OK;
    }
    return memcmp(rhs->buf, lhs->buf, rhs->sz) == 0 ? jmzk_OK : jmzk_DATA_NOT_EQUALS;
}

int
jmzk_version() {
    auto ver = jmzk::chain::contracts::jmzk_contract_abi_version();
    return ver.get_version();
}

thread_local int last_error_code = 0;

int
jmzk_set_last_error(int code) {
    last_error_code = code;
    return code;
}

int
jmzk_last_error() {
    return last_error_code;
}

}  // extern "C"