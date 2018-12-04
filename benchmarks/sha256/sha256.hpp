#include <stdio.h>
#include <string.h>
#include <stdint.h>

namespace sha256 {

namespace intrinsics {
void hash(const char* input, size_t len, uint32_t result[8]);
}

// namespace cryptopp {
// void hash(const char* input, size_t len, uint32_t result[8]);
// }

namespace fc {
void hash(const char* input, size_t len, uint32_t result[8]);
}

namespace cgminer {
void hash(const char* input, size_t len, uint32_t result[8]);
}

} // namespace sha256::intrinsics