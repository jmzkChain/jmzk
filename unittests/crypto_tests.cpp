#include <catch/catch.hpp>

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/private_key.hpp>

using namespace fc::crypto;

TEST_CASE("test_ecdh", "[ecdh]") {
    auto privkey1 = private_key::generate();
    auto privkey2 = private_key::generate();

    auto pubkey1 = privkey1.get_public_key();
    auto pubkey2 = privkey2.get_public_key();

    auto shared1 = privkey1.generate_shared_secret(pubkey2);
    auto shared2 = privkey2.generate_shared_secret(pubkey1);

    CHECK(shared1 == shared2);
}
