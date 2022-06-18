#include <iostream>

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

#include <libjmzk/jmzk_ecc.h>
#include <libjmzk/jmzk_abi.h>
#include <libjmzk/jmzk_address.h>
#include <libjmzk/jmzk_jmzklink.h>

using Catch::Matchers::Equals;

CATCH_TRANSLATE_EXCEPTION(fc::exception& e) {
    return e.to_detail_string();
}

void
dump_mem(jmzk_data_t* data) {
    printf("size: %ld, data: ", data->sz);
    char* ptr = (char*)data->buf;
    for(size_t i = 0; i < data->sz; i++, ptr++) {
        printf(" %02hhx", (unsigned char)*ptr);
    }
    printf("\n");
}

TEST_CASE("jmzklink") {
    void* linkp = nullptr;
    linkp = jmzk_link_new();
    REQUIRE(linkp != nullptr);
    
    auto r1 = jmzk_link_parse_from_jmzkli("03XBY4E/KTS:PNHVA3JP9QG258F08JHYOYR5SLJGN0EA-C3J6S:2G:T1SX7WA14KH9ETLZ97TUX9R9JJA6+06$E/_PYNX-/152P4CTC:WKXLK$/7G-K:89+::2K4C-KZ2**HI-P8CYJ**XGFO1K5:$E*SOY8MFYWMNHP*BHX2U8$$FTFI81YDP1HT",
        linkp);
    REQUIRE(r1 == jmzk_OK);
    REQUIRE(linkp != nullptr);
    
    uint16_t header;
    auto r2 = jmzk_link_get_header(linkp, &header);
    REQUIRE(r2 == jmzk_OK);
    REQUIRE(header == 3);

    uint32_t intv;
    char* strv;
    auto r3 = jmzk_link_get_segment_int(linkp, 42, &intv);
    REQUIRE(r3 == jmzk_OK);
    REQUIRE(intv == 1532465234);

    auto r4 = jmzk_link_get_segment_str(linkp, 91, &strv);
    REQUIRE(r4 == jmzk_OK);
    REQUIRE_THAT(strv, Equals("nd1532465232490"));
    
    auto r5 = jmzk_link_tostring(linkp, &strv);
    REQUIRE(r5 == jmzk_OK);
    REQUIRE_THAT(strv, Equals("03XBY4E/KTS:PNHVA3JP9QG258F08JHYOYR5SLJGN0EA-C3J6S:2G:T1SX7WA14KH9ETLZ97TUX9R9JJA6+06$E/_PYNX-/152P4CTC:WKXLK$/7G-K:89+::2K4C-KZ2**HI-P8CYJ**XGFO1K5:$E*SOY8MFYWMNHP*BHX2U8$$FTFI81YDP1HT"));

    uint32_t len = 0;
    jmzk_signature_t** signs = nullptr;
    auto r6 = jmzk_link_get_signatures(linkp, &signs, &len);
    REQUIRE(r6 == jmzk_OK);
    REQUIRE(signs != nullptr);
    REQUIRE(len == 1);
    char* sign_str;
    jmzk_signature_string(signs[0], &sign_str);
    REQUIRE_THAT(sign_str, Equals("SIG_K1_JyyaM7x9a4AjaD8yaG6iczgHskUFPvkWEk7X5DPkdZfRGBxYTbpLJ1y7gvmeL4vMqrMmw6QwtErfKUds5L7sxwU2nR7mvu"));

    jmzk_public_key_t*  pubkey = nullptr;
    jmzk_private_key_t* privkey = nullptr;
    jmzk_generate_new_pair(&pubkey, &privkey);
    auto r7 = jmzk_link_sign(linkp, privkey);
    REQUIRE(r7 == jmzk_OK);
    jmzk_link_get_signatures(linkp, &signs, &len);
    REQUIRE(len == 2);

    jmzk_link_free(linkp);
}

TEST_CASE("jmzkaddress") {
    jmzk_address_t* addr = nullptr;
    auto r11 = jmzk_address_reserved(&addr);
    REQUIRE(r11 == jmzk_OK);
    REQUIRE(addr != nullptr);
    char* type = nullptr;
    auto r111 = jmzk_address_get_type(addr, &type);
    REQUIRE(r111 == jmzk_OK);
    REQUIRE(std::string("reserved")==std::string(type));

    addr = nullptr;
    auto str = "jmzk6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3";

    jmzk_public_key_t* pub_key;
    auto r121 = jmzk_public_key_from_string(str, &pub_key);
    REQUIRE(r121 == jmzk_OK);
    REQUIRE(pub_key != nullptr);

    auto r12 = jmzk_address_public_key(pub_key, &addr);
    REQUIRE(r12 == jmzk_OK);
    REQUIRE(addr != nullptr);

    auto r122 = jmzk_address_get_type(addr, &type);
    REQUIRE(r122 == jmzk_OK);
    REQUIRE(std::string("public_key")==std::string(type));

    addr = nullptr;
    auto r13 = jmzk_address_generated("jmzk", "jmzkChain", 8888, &addr);
    REQUIRE(r13 == jmzk_OK);
    REQUIRE(addr != nullptr);
    
    auto r131 = jmzk_address_get_type(addr, &type);
    REQUIRE(r131 == jmzk_OK);
    REQUIRE_THAT(type, Equals("generated"));

    addr = nullptr;
    auto r2 = jmzk_address_from_string(str, &addr);
    REQUIRE(r2 == jmzk_OK);
    REQUIRE(addr != nullptr);

    char* ret;
    auto r3 = jmzk_address_to_string(addr, &ret);
    REQUIRE(r3 == jmzk_OK);
    INFO("lhs is " << str);
    INFO("rhs is " << ret);
    REQUIRE_THAT(str, Equals(ret));
}

TEST_CASE("jmzkecc") {
    jmzk_public_key_t*  pubkey = nullptr;
    jmzk_private_key_t* privkey = nullptr;
    auto r1 = jmzk_generate_new_pair(&pubkey, &privkey);
    REQUIRE(r1 == jmzk_OK);
    REQUIRE(pubkey != nullptr);
    REQUIRE(privkey != nullptr);

    char* privkey_str;
    auto r11 = jmzk_private_key_string(privkey, &privkey_str);
    CHECK(r11 == jmzk_OK);

    jmzk_private_key_t* privkey2 = nullptr;
    auto r12 = jmzk_private_key_from_string(privkey_str, &privkey2);
    CHECK(r12 == jmzk_OK);
    CHECK(jmzk_equals(privkey, privkey2) == jmzk_OK);

    jmzk_public_key_t* pubkey2 = nullptr;
    auto r2 = jmzk_get_public_key(privkey, &pubkey2);
    REQUIRE(r2 == jmzk_OK);
    REQUIRE(pubkey2 != nullptr);

    char* pubkey1_str, *pubkey2_str;
    auto r21 = jmzk_public_key_string(pubkey, &pubkey1_str);
    auto r22 = jmzk_public_key_string(pubkey2, &pubkey2_str);
    CHECK(r21 == jmzk_OK);
    CHECK(r22 == jmzk_OK);
    INFO("lhs is " << pubkey1_str);
    INFO("rhs is " << pubkey2_str);
    REQUIRE(jmzk_equals(pubkey, pubkey2) == jmzk_OK);

    jmzk_public_key_t* pubkey4 = nullptr;
    auto r23 = jmzk_public_key_from_string(pubkey1_str, &pubkey4);
    CHECK(r23 == jmzk_OK);
    CHECK(jmzk_equals(pubkey, pubkey4) == jmzk_OK);

    auto str = "jmzk";
    jmzk_checksum_t* hash = nullptr;
    auto r3 = jmzk_hash((const char*)str, sizeof(str), &hash);
    REQUIRE(r3 == jmzk_OK);
    REQUIRE(hash != nullptr);

    char* hash_str;
    auto r31 = jmzk_checksum_string(hash, &hash_str);
    CHECK(r31 == jmzk_OK);

    jmzk_checksum_t* hash2 = nullptr;
    auto r32 = jmzk_checksum_from_string(hash_str, &hash2);
    CHECK(r32 == jmzk_OK);
    CHECK(jmzk_equals(hash, hash2) == jmzk_OK);

    jmzk_signature_t* sign = nullptr;
    auto r4 = jmzk_sign_hash(privkey, hash, &sign);
    REQUIRE(r4 == jmzk_OK);
    REQUIRE(sign != nullptr);

    char* sign_str;
    auto r41 = jmzk_signature_string(sign, &sign_str);
    CHECK(r41 == jmzk_OK);

    jmzk_signature_t* sign2 = nullptr;
    auto r42 = jmzk_signature_from_string(sign_str, &sign2);
    CHECK(r42 == jmzk_OK);
    CHECK(jmzk_equals(sign, sign2) == jmzk_OK);

    jmzk_public_key_t* pubkey3 = nullptr;
    auto r5 = jmzk_recover(sign, hash, &pubkey3);
    REQUIRE(r5 == jmzk_OK);
    REQUIRE(pubkey3 != nullptr);
    REQUIRE(jmzk_equals(pubkey, pubkey3) == jmzk_OK);

    jmzk_free(pubkey);
    jmzk_free(privkey);
    jmzk_free(pubkey2);
    jmzk_free(pubkey1_str);
    jmzk_free(pubkey2_str);
    jmzk_free(pubkey4);
    jmzk_free(hash);
    jmzk_free(hash_str);
    jmzk_free(hash2);
    jmzk_free(sign);
    jmzk_free(sign_str);
    jmzk_free(sign2);
    jmzk_free(pubkey3);
}

TEST_CASE("jmzkabi") {
    auto abi = jmzk_abi();
    REQUIRE(abi != nullptr);

    auto j1 = R"(
    {
        "name": "RD0G5W3jPw",
        "creator": "jmzk6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
        "issue": {
            "name": "issue",
            "threshold": 1,
            "authorizers": [
                {
                    "ref": "[A] jmzk6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
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
                    "ref": "[A] jmzk6QqRegP6k3ot13kMwUjz5aad1F1SaizoeBPqh1ge9iGEeUaZa7",
                    "weight": 1
                }
            ]
        }
    }
    )";

    jmzk_bin_t* bin = nullptr;
    auto r1 = jmzk_abi_json_to_bin(abi, "newdomain", j1, &bin);
    REQUIRE(r1 == jmzk_OK);
    REQUIRE(bin != nullptr);
    REQUIRE(bin->sz > 0);

    jmzk_bin_t* bin2 = nullptr;
    auto r11 = jmzk_abi_json_to_bin(abi, "newdomain", "newdomain", &bin2);
    REQUIRE(r11 == jmzk_INVALID_JSON);
    REQUIRE(bin2 == nullptr);

    char* j1restore = nullptr;
    auto r2 = jmzk_abi_bin_to_json(abi, "newdomain", bin, &j1restore);
    REQUIRE(r2 == jmzk_OK);
    REQUIRE(j1restore != nullptr);

    auto sz = strlen(j1restore);
    CHECK(j1restore[sz] == '\0');
    CHECK(j1restore[sz-1] == '}');

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

    jmzk_chain_id_t* chain_id = nullptr;
    auto r3 = jmzk_chain_id_from_string("bb248d6319e51ad38502cc8ef8fe607eb5ad2cd0be2bdc0e6e30a506761b8636", &chain_id);
    REQUIRE(r3 == jmzk_OK);
    REQUIRE(chain_id != nullptr);

    jmzk_checksum_t* digest = nullptr;
    auto r4 = jmzk_trx_json_to_digest(abi, j2, chain_id, &digest);
    REQUIRE(r4 == jmzk_OK);
    REQUIRE(digest != nullptr);

    jmzk_block_id_t* block_id = nullptr;
    auto r5 = jmzk_block_id_from_string("000000cabd11d7f8163d5586a4bb4ef6bb8d0581f03db67a04c285bbcb83f921", &block_id);
    REQUIRE(r5 == jmzk_OK);
    REQUIRE(block_id != nullptr);

    uint16_t ref_block_num = 0;
    auto r6 = jmzk_ref_block_num(block_id, &ref_block_num);
    REQUIRE(r6 == jmzk_OK);
    CHECK(ref_block_num == 202);

    uint32_t ref_block_prefix = 0;
    auto r7 = jmzk_ref_block_prefix(block_id, &ref_block_prefix);
    REQUIRE(r7 == jmzk_OK);
    CHECK(ref_block_prefix == 2253733142);

    auto j3 = R"(
    {
        "name": "test1530718665",
        "signatures": [
            "SIG_K1_KXjtmeihJi1qnSs7vmqJDRJoZ1nSEPeeRjsKJRpm24g8yhFtAepkRDR4nVFbXjvoaQvT4QrzuNWCbuEhceYpGmAvsG47Fj"
        ]
    }
    )";
    jmzk_bin_t* bin3 = nullptr;
    auto r8 = jmzk_abi_json_to_bin(abi, "aprvsuspend", j3, &bin3);
    REQUIRE(r8 == jmzk_OK);

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
    jmzk_checksum_t* digest2 = nullptr;
    auto r9 = jmzk_trx_json_to_digest(abi, j4, chain_id, &digest2);
    REQUIRE(r9 == jmzk_OK);
    REQUIRE(digest2 != nullptr);

    REQUIRE(abi != nullptr);

    jmzk_free(bin);
    jmzk_free(j1restore);
    jmzk_free(chain_id);
    jmzk_free(digest);
    jmzk_free(digest2);
    jmzk_free(block_id);
    jmzk_free(bin3);
    jmzk_free_abi(abi);
}
