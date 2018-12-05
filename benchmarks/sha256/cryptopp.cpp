#include "sha256.hpp"
#include <cryptopp/sha.h>

namespace sha256 { namespace cryptopp {

void
hash(const char* input, size_t len, uint32_t result[8]) {
    CryptoPP::SHA256::InitState(result);
    for(auto i = 0u; i < len / 64; i++, input += 64) {
        CryptoPP::SHA256::Transform(result, (uint32_t*)input);
    }
}

}}  // namespace cryptopp