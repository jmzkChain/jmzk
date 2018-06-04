#define BOOST_TEST_MODULE libevt test

#include <boost/test/included/unit_test.hpp>
#include <libevt/evt_ecc.h>
#include <iostream>

void
dump_mem(evt_data_t* data) {
    printf("size: %ld, data: ", data->sz);
    char* ptr = (char*)data->buf;
    for(size_t i = 0; i < data->sz; i++, ptr++) {
        printf(" %02hhx", (unsigned char)*ptr);
    }
    printf("\n");
}

BOOST_AUTO_TEST_CASE( crypto ) {
    evt_public_key_t*  pubkey = nullptr;
    evt_private_key_t* privkey = nullptr;
    auto r1 = evt_generate_new_pair(&pubkey, &privkey);
    BOOST_TEST_REQUIRE(r1 == EVT_OK);
    BOOST_TEST_REQUIRE(pubkey != nullptr);
    BOOST_TEST_REQUIRE(privkey != nullptr);

    char* privkey_str;
    auto r11 = evt_private_key_string(privkey, &privkey_str);
    BOOST_TEST_CHECK(r11 == EVT_OK);

    evt_public_key_t* pubkey2 = nullptr;
    auto r2 = evt_get_public_key(privkey, &pubkey2);
    BOOST_TEST_REQUIRE(r2 == EVT_OK);
    BOOST_TEST_REQUIRE(pubkey2 != nullptr);

    char* pubkey1_str, *pubkey2_str;
    auto r21 = evt_public_key_string(pubkey, &pubkey1_str);
    auto r22 = evt_public_key_string(pubkey2, &pubkey2_str);
    BOOST_TEST_CHECK(r21 == EVT_OK);
    BOOST_TEST_CHECK(r22 == EVT_OK);
    BOOST_TEST_REQUIRE(evt_equals(pubkey, pubkey2) == EVT_OK, "\nlhs is " << pubkey1_str << "\nrhs is " << pubkey2_str);

    auto str = "evt";
    evt_checksum_t* hash = nullptr;
    auto r3 = evt_hash((const char*)str, sizeof(str), &hash);
    BOOST_TEST_REQUIRE(r3 == EVT_OK);
    BOOST_TEST_REQUIRE(hash != nullptr);

    char* hash_str;
    auto r31 = evt_checksum_string(hash, &hash_str);
    BOOST_TEST_CHECK(r31 == EVT_OK);

    evt_signature_t* sign = nullptr;
    auto r4 = evt_sign_hash(privkey, hash, &sign);
    BOOST_TEST_REQUIRE(r4 == EVT_OK);
    BOOST_TEST_REQUIRE(sign != nullptr);

    char* sign_str;
    auto r41 = evt_signature_string(sign, &sign_str);
    BOOST_TEST_CHECK(r41 == EVT_OK);

    evt_public_key_t* pubkey3 = nullptr;
    auto r5 = evt_recover(sign, hash, &pubkey3);
    BOOST_TEST_REQUIRE(r5 == EVT_OK);
    BOOST_TEST_REQUIRE(pubkey3 != nullptr);
    BOOST_TEST_REQUIRE(evt_equals(pubkey, pubkey3) == EVT_OK);

    evt_free(pubkey);
    evt_free(privkey);
    evt_free(pubkey2);
    evt_free(hash);
    evt_free(sign);
    evt_free(pubkey3);
}
