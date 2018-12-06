#include "sha256.hpp"
#include <fc/crypto/sha256.hpp>

namespace sha256 { namespace fc {

void
hash(const char* input, size_t len, uint32_t result[8]) {
    auto h = ::fc::sha256::hash(input, len);
    memcpy(result, &h, sizeof(h));
}

}}  // namespace sha256::fc