#define BOOST_TEST_MODULE libevt test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <libevt/evt_ecc.h>
#include <libevt/evt_abi.h>
#include <libevt/evt_address.h>
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

BOOST_AUTO_TEST_CASE( evtaddress ) {
    evt_address_t* addr = nullptr;
    auto r11 = evt_address_reserved(&addr);
    BOOST_TEST_REQUIRE(r11 == EVT_OK);
    BOOST_TEST_REQUIRE(addr != nullptr);
    char* type = nullptr;
    auto r111 = evt_address_get_type(addr, &type);
    BOOST_TEST_REQUIRE(r111 == EVT_OK);
    BOOST_TEST_REQUIRE(std::string("reserved")==std::string(type));

    addr = nullptr;
    auto str = "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3";
    evt_public_key_t* pub_key;
    auto r121 = evt_public_key_from_string(str, &pub_key);
    auto r12 = evt_address_public_key(pub_key, &addr);
    BOOST_TEST_REQUIRE(r12 == EVT_OK);
    BOOST_TEST_REQUIRE(addr != nullptr);
    auto r122 = evt_address_get_type(addr, &type);
    BOOST_TEST_REQUIRE(r122 == EVT_OK);
    BOOST_TEST_REQUIRE(std::string("public_key")==std::string(type));

    addr = nullptr;
    auto r13 = evt_address_generated("evt", "everitoken", 8888, &addr);
    BOOST_TEST_REQUIRE(r13 == EVT_OK);
    BOOST_TEST_REQUIRE(addr != nullptr);
    auto r131 = evt_address_get_type(addr, &type);
    BOOST_TEST_REQUIRE(r131 == EVT_OK);
    BOOST_TEST_REQUIRE(std::string("generated")==std::string(type));

    addr = nullptr;
    auto r2 = evt_address_from_string(str, &addr);
    BOOST_TEST_REQUIRE(r2 == EVT_OK);
    BOOST_TEST_REQUIRE(addr != nullptr);

    char* ret;
    auto r3 = evt_address_to_string(addr, &ret);
    BOOST_TEST_REQUIRE(r3 == EVT_OK);
    BOOST_TEST_REQUIRE(std::string(str) == std::string(ret), "\nlhs is " << str << "\nrhs is " << ret);
}

BOOST_AUTO_TEST_CASE( evtecc ) {
    evt_public_key_t*  pubkey = nullptr;
    evt_private_key_t* privkey = nullptr;
    auto r1 = evt_generate_new_pair(&pubkey, &privkey);
    BOOST_TEST_REQUIRE(r1 == EVT_OK);
    BOOST_TEST_REQUIRE(pubkey != nullptr);
    BOOST_TEST_REQUIRE(privkey != nullptr);

    char* privkey_str;
    auto r11 = evt_private_key_string(privkey, &privkey_str);
    BOOST_TEST_CHECK(r11 == EVT_OK);

    evt_private_key_t* privkey2 = nullptr;
    auto r12 = evt_private_key_from_string(privkey_str, &privkey2);
    BOOST_TEST_CHECK(r12 == EVT_OK);
    BOOST_TEST_CHECK(evt_equals(privkey, privkey2) == EVT_OK);

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

    evt_public_key_t* pubkey4 = nullptr;
    auto r23 = evt_public_key_from_string(pubkey1_str, &pubkey4);
    BOOST_TEST_CHECK(r23 == EVT_OK);
    BOOST_TEST_CHECK(evt_equals(pubkey, pubkey4) == EVT_OK);

    auto str = "evt";
    evt_checksum_t* hash = nullptr;
    auto r3 = evt_hash((const char*)str, sizeof(str), &hash);
    BOOST_TEST_REQUIRE(r3 == EVT_OK);
    BOOST_TEST_REQUIRE(hash != nullptr);

    char* hash_str;
    auto r31 = evt_checksum_string(hash, &hash_str);
    BOOST_TEST_CHECK(r31 == EVT_OK);

    evt_checksum_t* hash2 = nullptr;
    auto r32 = evt_checksum_from_string(hash_str, &hash2);
    BOOST_TEST_CHECK(r32 == EVT_OK);
    BOOST_TEST_CHECK(evt_equals(hash, hash2) == EVT_OK);

    evt_signature_t* sign = nullptr;
    auto r4 = evt_sign_hash(privkey, hash, &sign);
    BOOST_TEST_REQUIRE(r4 == EVT_OK);
    BOOST_TEST_REQUIRE(sign != nullptr);

    char* sign_str;
    auto r41 = evt_signature_string(sign, &sign_str);
    BOOST_TEST_CHECK(r41 == EVT_OK);

    evt_signature_t* sign2 = nullptr;
    auto r42 = evt_signature_from_string(sign_str, &sign2);
    BOOST_TEST_CHECK(r42 == EVT_OK);
    BOOST_TEST_CHECK(evt_equals(sign, sign2) == EVT_OK);

    evt_public_key_t* pubkey3 = nullptr;
    auto r5 = evt_recover(sign, hash, &pubkey3);
    BOOST_TEST_REQUIRE(r5 == EVT_OK);
    BOOST_TEST_REQUIRE(pubkey3 != nullptr);
    BOOST_TEST_REQUIRE(evt_equals(pubkey, pubkey3) == EVT_OK);

    evt_free(pubkey);
    evt_free(privkey);
    evt_free(pubkey2);
    evt_free(pubkey1_str);
    evt_free(pubkey2_str);
    evt_free(pubkey4);
    evt_free(hash);
    evt_free(hash_str);
    evt_free(hash2);
    evt_free(sign);
    evt_free(sign_str);
    evt_free(sign2);
    evt_free(pubkey3);
}

BOOST_AUTO_TEST_CASE( evtabi ) {
    auto abi = evt_abi();
    BOOST_TEST_REQUIRE(abi != nullptr);

    auto j1 = R"(
    {
        "name": "RD0G5W3jPw",
        "creator": "EVT6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
        "issue": {
            "name": "issue",
            "threshold": 1,
            "authorizers": [
                {
                    "ref": "[A] EVT6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
                    "weight": 1
                }
            ]
        },
        "transfer": {
            "name": "transfer",
            "threshold": 1,
            "authorizers": [
                {
                    "ref": "[G] OWNER",
                    "weight": 1
                }
            ]
        },
        "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [
                {
                    "ref": "[A] EVT6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
                    "weight": 1
                }
            ]
        }
    }
    )";

    evt_bin_t* bin = nullptr;
    auto r1 = evt_abi_json_to_bin(abi, "newdomain", j1, &bin);
    BOOST_TEST_REQUIRE(r1 == EVT_OK);
    BOOST_TEST_REQUIRE(bin != nullptr);
    BOOST_TEST_REQUIRE(bin->sz > 0);

    evt_bin_t* bin2 = nullptr;
    auto r11 = evt_abi_json_to_bin(abi, "newdomain", "newdomain", &bin2);
    BOOST_TEST_REQUIRE(r11 == EVT_INVALID_JSON);
    BOOST_TEST_REQUIRE(bin2 == nullptr);

    char* j1restore = nullptr;
    auto r2 = evt_abi_bin_to_json(abi, "newdomain", bin, &j1restore);
    BOOST_TEST_REQUIRE(r2 == EVT_OK);
    BOOST_TEST_REQUIRE(j1restore != nullptr);

    auto sz = strlen(j1restore);
    BOOST_TEST_CHECK(j1restore[sz] == '\0');
    BOOST_TEST_CHECK(j1restore[sz-1] == '}');

    auto j2 = R"(
    {
        "expiration": "2018-05-20T12:25:51",
        "ref_block_num": 8643,
        "ref_block_prefix": 842752750,
        "actions": [
            {
                "name": "newdomain",
                "domain": "domain",
                "key": "test2",
                "data": "000000000000000000000000109f077d0003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000a5317601000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100000000572d3ccdcd010000000102000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000002866a69101000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100"
            }
        ],
        "transaction_extensions": []
    }
    )";

    evt_chain_id_t* chain_id = nullptr;
    auto r3 = evt_chain_id_from_string("bb248d6319e51ad38502cc8ef8fe607eb5ad2cd0be2bdc0e6e30a506761b8636", &chain_id);
    BOOST_TEST_REQUIRE(r3 == EVT_OK);
    BOOST_TEST_REQUIRE(chain_id != nullptr);

    evt_checksum_t* digest = nullptr;
    auto r4 = evt_trx_json_to_digest(abi, j2, chain_id, &digest);
    BOOST_TEST_REQUIRE(r4 == EVT_OK);
    BOOST_TEST_REQUIRE(digest != nullptr);

    evt_block_id_t* block_id = nullptr;
    auto r5 = evt_block_id_from_string("000000cabd11d7f8163d5586a4bb4ef6bb8d0581f03db67a04c285bbcb83f921", &block_id);
    BOOST_TEST_REQUIRE(r5 == EVT_OK);
    BOOST_TEST_REQUIRE(block_id != nullptr);

    uint16_t ref_block_num = 0;
    auto r6 = evt_ref_block_num(block_id, &ref_block_num);
    BOOST_TEST_REQUIRE(r6 == EVT_OK);
    BOOST_TEST_CHECK(ref_block_num == 202);

    uint32_t ref_block_prefix = 0;
    auto r7 = evt_ref_block_prefix(block_id, &ref_block_prefix);
    BOOST_TEST_REQUIRE(r7 == EVT_OK);
    BOOST_TEST_CHECK(ref_block_prefix == 2253733142);

    auto j3 = R"(
    {
        "name": "test1530718665",
        "signatures": [
            "SIG_K1_KXjtmeihJi1qnSs7vmqJDRJoZ1nSEPeeRjsKJRpm24g8yhFtAepkRDR4nVFbXjvoaQvT4QrzuNWCbuEhceYpGmAvsG47Fj"
        ]
    }
    )";
    evt_bin_t* bin3 = nullptr;
    auto r8 = evt_abi_json_to_bin(abi, "aprvsuspend", j3, &bin3);
    BOOST_TEST_REQUIRE(r8 == EVT_OK);

    auto j4 = R"(
    {
        "expiration": "2018-07-11T02:48:54",
        "ref_block_num": "58678",
        "ref_block_prefix": "2495876290",
        "actions": [
            {
                "name": "issuetoken",
                "domain": "JFaL0nLyip",
                "key": ".issue",
                "data": "0000000000000000b051649c0931b3be01000000000000c4f0776ff9fa6490a57d010003e6cc7f10174005461fe73b8051dad4e5858b77176f22db6ebfd15fb19d414984"
            }
        ],
        "transaction_extensions": []
    }
    )";
    evt_checksum_t* digest2 = nullptr;
    auto r9 = evt_trx_json_to_digest(abi, j4, chain_id, &digest2);
    BOOST_TEST_REQUIRE(r9 == EVT_OK);
    BOOST_TEST_REQUIRE(digest2 != nullptr);

    evt_free(bin);
    evt_free(j1restore);
    evt_free(chain_id);
    evt_free(digest);
    evt_free(digest2);
    evt_free(block_id);
    evt_free(bin3);
    evt_free_abi(abi);
}
