#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <vector>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <fc/variant.hpp>

#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/types.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;

// BOOST_AUTO_TEST_SUITE(unittests)

// verify that round trip conversion, via bytes, reproduces the exact same data
fc::variant
verify_byte_round_trip_conversion(const abi_serializer& abis, const type_name& type, const fc::variant& var) {
    auto bytes = abis.variant_to_binary(type, var);

    auto var2 = abis.binary_to_variant(type, bytes);

    std::string r = fc::json::to_string(var2);

    auto bytes2 = abis.variant_to_binary(type, var2);

    BOOST_TEST(fc::to_hex(bytes) == fc::to_hex(bytes2));

    return var2;
}

auto
get_evt_abi() {
    static auto abis = abi_serializer(evt_contract_abi());
    return abis;
}

// verify that round trip conversion, via actual class, reproduces the exact same data
template <typename T>
fc::variant
verify_type_round_trip_conversion(const abi_serializer& abis, const type_name& type, const fc::variant& var) {
    auto bytes = abis.variant_to_binary(type, var);

    T obj;
    fc::from_variant(var, obj);

    fc::variant var2;
    fc::to_variant(obj, var2);

    std::string r = fc::json::to_string(var2);

    auto bytes2 = abis.variant_to_binary(type, var2);

    BOOST_TEST(bytes.size() == bytes2.size());
    BOOST_TEST(fc::to_hex(bytes) == fc::to_hex(bytes2));

    return var2;
}

struct optionaltest {
    fc::optional<int> a;
    fc::optional<int> b;
};
FC_REFLECT(optionaltest, (a)(b));

struct optionaltest2 {
    fc::optional<optionaltest> a;
    fc::optional<optionaltest> b;
};
FC_REFLECT(optionaltest2, (a)(b));

BOOST_AUTO_TEST_CASE(optional_test) {
    try {
        auto abi = abi_def();
        abi.structs.emplace_back( struct_def {
            "optionaltest", "", {
                {"a", "int32?"},
                {"b", "int32?"}
            }
        });
        abi.structs.emplace_back( struct_def {
            "optionaltest2", "", {
                {"a", "optionaltest?"},
                {"b", "optionaltest?"}
            }
        });

        auto abis = abi_serializer(abi);

        auto json = R"( { "a": 0 } )";
        auto json2 = R"( {"a": { "a": 0 } } )";
        auto var1 = fc::json::from_string(json);
        auto var2 = fc::json::from_string(json2);
        auto bytes1  = abis.variant_to_binary("optionaltest", var1);
        auto bytes2 = abis.variant_to_binary("optionaltest2", var2);

        BOOST_TEST(var1["a"].is_integer());
        BOOST_CHECK_THROW(var1["b"].is_null(), fc::key_not_found_exception);

        BOOST_TEST((var2["a"].is_object() && var2["a"].get_object().size() > 0));
        BOOST_CHECK_THROW((var2["b"].is_object() && var2["b"].get_object().size() == 0), fc::key_not_found_exception);

        optionaltest ot;
        fc::from_variant(var1, ot);

        optionaltest2 ot2;
        fc::from_variant(var2, ot2);

        BOOST_CHECK(ot.a.valid());
        BOOST_CHECK(!ot.b.valid());

        BOOST_CHECK(ot2.a.valid());
        BOOST_CHECK(!ot2.b.valid());

        fc::variant var21, var22;
        fc::to_variant(ot, var21);
        fc::to_variant(ot2, var22);

        BOOST_TEST(var21["a"].is_integer());
        BOOST_CHECK_THROW(var21["b"].is_null(), fc::key_not_found_exception);

        BOOST_TEST((var22["a"].is_object() && var22["a"].get_object().size() > 0));
        BOOST_CHECK_THROW((var22["b"].is_object() && var22["b"].get_object().size() == 0), fc::key_not_found_exception);  

        auto bytes21 = abis.variant_to_binary("optionaltest", var21);
        BOOST_TEST(fc::to_hex(bytes1) == fc::to_hex(bytes21));

        auto bytes22 = abis.variant_to_binary("optionaltest2", var22);
        BOOST_TEST(fc::to_hex(bytes2) == fc::to_hex(bytes22));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newdomain_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "cookie",
          "creator" : "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "issue" : {
            "name" : "issue",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                "weight": 1
              }
            ]
          },
          "transfer": {
            "name": "transfer",
            "threshold": 1,
            "authorizers": [{
                "ref": "[G] OWNER",
                "weight": 1
              }
            ]
          },
          "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                "weight": 1
              }
            ]
          }
        }
        )=====";

        auto var    = fc::json::from_string(test_data);
        auto newdom = var.as<newdomain>();
        BOOST_TEST("cookie" == newdom.name);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newdom.creator);

        BOOST_TEST("issue" == newdom.issue.name);
        BOOST_TEST(1 == newdom.issue.threshold);
        BOOST_TEST_REQUIRE(1 == newdom.issue.authorizers.size());
        BOOST_TEST(newdom.issue.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newdom.issue.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newdom.issue.authorizers[0].weight);

        BOOST_TEST("transfer" == newdom.transfer.name);
        BOOST_TEST(1 == newdom.transfer.threshold);
        BOOST_TEST_REQUIRE(1 == newdom.transfer.authorizers.size());
        BOOST_TEST(newdom.transfer.authorizers[0].ref.is_owner_ref());
        BOOST_TEST(1 == newdom.transfer.authorizers[0].weight);

        BOOST_TEST("manage" == newdom.manage.name);
        BOOST_TEST(1 == newdom.manage.threshold);
        BOOST_TEST_REQUIRE(1 == newdom.manage.authorizers.size());
        BOOST_TEST(newdom.manage.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newdom.manage.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newdom.manage.authorizers[0].weight);

        auto var2    = verify_byte_round_trip_conversion(abis, "newdomain", var);
        auto newdom2 = var2.as<newdomain>();
        BOOST_TEST(newdom2.name == newdom.name);
        BOOST_TEST((std::string)newdom2.creator == (std::string)newdom.creator);

        BOOST_TEST(newdom2.issue.name == newdom.issue.name);
        BOOST_TEST(newdom2.issue.threshold == newdom.issue.threshold);
        BOOST_TEST_REQUIRE(newdom2.issue.authorizers.size() == newdom.issue.authorizers.size());
        BOOST_TEST(newdom2.issue.authorizers[0].ref.type() == newdom.issue.authorizers[0].ref.type());
        BOOST_TEST((std::string)newdom2.issue.authorizers[0].ref.get_account() == (std::string)newdom.issue.authorizers[0].ref.get_account());
        BOOST_TEST(newdom2.issue.authorizers[0].weight == newdom.issue.authorizers[0].weight);

        BOOST_TEST(newdom2.transfer.name == newdom.transfer.name);
        BOOST_TEST(newdom2.transfer.threshold == newdom.transfer.threshold);
        BOOST_TEST_REQUIRE(newdom2.transfer.authorizers.size() == newdom.transfer.authorizers.size());
        BOOST_TEST(newdom2.transfer.authorizers[0].ref.type() == newdom.transfer.authorizers[0].ref.type());
        BOOST_TEST(newdom2.transfer.authorizers[0].weight == newdom.transfer.authorizers[0].weight);

        BOOST_TEST(newdom2.manage.name == newdom.manage.name);
        BOOST_TEST(newdom2.manage.threshold == newdom.manage.threshold);
        BOOST_TEST_REQUIRE(newdom2.manage.authorizers.size() == newdom.manage.authorizers.size());
        BOOST_TEST(newdom2.transfer.authorizers[0].ref.type() == newdom.transfer.authorizers[0].ref.type());
        BOOST_TEST((std::string)newdom2.manage.authorizers[0].ref.get_account() == (std::string)newdom.manage.authorizers[0].ref.get_account());
        BOOST_TEST(newdom2.manage.authorizers[0].weight == newdom.manage.authorizers[0].weight);

        verify_type_round_trip_conversion<newdomain>(abis, "newdomain", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(updatedomain_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "cookie",
          "issue" : {
            "name": "issue",
            "threshold": 2,
            "authorizers": [{
              "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
              "weight": 1},{
                "ref": "[G] new-group",
                "weight": 1
              } ]
          }
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto updom = var.as<updatedomain>();

        BOOST_TEST("cookie" == updom.name);

        BOOST_TEST("issue" == updom.issue->name);
        BOOST_TEST(2 == updom.issue->threshold);
        BOOST_TEST_REQUIRE(2 == updom.issue->authorizers.size());
        BOOST_TEST_REQUIRE(updom.issue->authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)updom.issue->authorizers[0].ref.get_account());
        BOOST_TEST(1 == updom.issue->authorizers[0].weight);

        auto var2   = verify_byte_round_trip_conversion(abis, "updatedomain", var);
        auto updom2 = var2.as<updatedomain>();

        BOOST_TEST("cookie" == updom2.name);

        BOOST_TEST("issue" == updom2.issue->name);
        BOOST_TEST(2 == updom2.issue->threshold);
        BOOST_TEST_REQUIRE(2 == updom2.issue->authorizers.size());
        BOOST_TEST_REQUIRE(updom2.issue->authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)updom2.issue->authorizers[0].ref.get_account());
        BOOST_TEST(1 == updom2.issue->authorizers[0].weight);

        verify_type_round_trip_conversion<updatedomain>(abis, "updatedomain", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(issuetoken_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "cookie",
            "names": [
              "t1",
              "t2",
              "t3"
            ],
            "owner": [
              "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
            ]
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto istk = var.as<issuetoken>();

        BOOST_TEST("cookie" == istk.domain);

        BOOST_TEST_REQUIRE(3 == istk.names.size());
        BOOST_TEST("t1" == istk.names[0]);
        BOOST_TEST("t2" == istk.names[1]);
        BOOST_TEST("t3" == istk.names[2]);

        BOOST_TEST_REQUIRE(1 == istk.owner.size());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)istk.owner[0]);

        auto var2  = verify_byte_round_trip_conversion(abis, "issuetoken", var);
        auto istk2 = var2.as<issuetoken>();

        BOOST_TEST("cookie" == istk2.domain);

        BOOST_TEST_REQUIRE(3 == istk2.names.size());
        BOOST_TEST("t1" == istk2.names[0]);
        BOOST_TEST("t2" == istk2.names[1]);
        BOOST_TEST("t3" == istk2.names[2]);

        BOOST_TEST_REQUIRE(1 == istk2.owner.size());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)istk2.owner[0]);

        verify_type_round_trip_conversion<issuetoken>(abis, "issuetoken", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "cookie",
          "name": "t1",
          "to": [
            "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
          ],
          "memo":"memo"
        }
        )=====";

        auto var = fc::json::from_string(test_data);
        auto trf = var.as<transfer>();

        BOOST_TEST("cookie" == trf.domain);
        BOOST_TEST("t1" == trf.name);

        BOOST_TEST_REQUIRE(1 == trf.to.size());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)trf.to[0]);
        BOOST_TEST("memo" == trf.memo);

        auto var2 = verify_byte_round_trip_conversion(abis, "transfer", var);
        auto trf2 = var2.as<transfer>();

        BOOST_TEST("cookie" == trf2.domain);
        BOOST_TEST("t1" == trf2.name);

        BOOST_TEST_REQUIRE(1 == trf2.to.size());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)trf2.to[0]);
        BOOST_TEST("memo" == trf2.memo);

        verify_type_round_trip_conversion<transfer>(abis, "transfer", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(destroytoken_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "cookie",
          "name": "t1"
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto destk = var.as<destroytoken>();

        BOOST_TEST("cookie" == destk.domain);
        BOOST_TEST("t1" == destk.name);

        auto var2   = verify_byte_round_trip_conversion(abis, "destroytoken", var);
        auto destk2 = var2.as<destroytoken>();

        BOOST_TEST("cookie" == destk2.domain);
        BOOST_TEST("t1" == destk2.name);

        verify_type_round_trip_conversion<transfer>(abis, "destroytoken", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newgroup_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "5jxX",
          "group" : {
            "name": "5jxXg",
            "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "root": {
              "threshold": 6,
              "weight": 0,
              "nodes": [{
                  "type": "branch",
                  "threshold": 1,
                  "weight": 3,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 1
                    }
                  ]
                },{
                  "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 3
                },{
                  "threshold": 1,
                  "weight": 3,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 2
                    }
                  ]
                }
              ]
            }
          }
        }
        )=====";

        auto var = fc::json::from_string(test_data);

        auto newgrp = var.as<newgroup>();
        BOOST_TEST("5jxX" == newgrp.name);

        BOOST_TEST("5jxXg" == newgrp.group.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp.group.key());

        auto root = newgrp.group.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(6 == root.threshold);
        BOOST_TEST(0 == root.weight);

        auto son0 = newgrp.group.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        auto son0_son0 = newgrp.group.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp.group.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        auto son0_son1 = newgrp.group.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp.group.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        auto son1 = newgrp.group.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp.group.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        auto son2 = newgrp.group.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        auto son2_son0 = newgrp.group.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp.group.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        auto son2_son1 = newgrp.group.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp.group.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);

        auto var2    = verify_byte_round_trip_conversion(abis, "newgroup", var);
        auto newgrp2 = var2.as<newgroup>();

        BOOST_TEST("5jxX" == newgrp2.name);

        BOOST_TEST("5jxXg" == newgrp2.group.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp2.group.key());

        root = newgrp2.group.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(6 == root.threshold);
        BOOST_TEST(0 == root.weight);

        son0 = newgrp2.group.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        son0_son0 = newgrp2.group.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp2.group.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        son0_son1 = newgrp2.group.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp2.group.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        son1 = newgrp2.group.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp2.group.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        son2 = newgrp2.group.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        son2_son0 = newgrp2.group.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newgrp2.group.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        son2_son1 = newgrp2.group.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)newgrp2.group.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);

        verify_type_round_trip_conversion<newgroup>(abis, "newgroup", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(updategroup_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "5jxX",
          "group" : {
            "name": "5jxXg",
            "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "root": {
              "threshold": 6,
              "weight": 0,
              "nodes": [{
                  "type": "branch",
                  "threshold": 1,
                  "weight": 3,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 1
                    }
                  ]
                },{
                  "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 3
                },{
                  "threshold": 1,
                  "weight": 3,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 2
                    }
                  ]
                }
              ]
            }
          }
        }
        )=====";

        auto var = fc::json::from_string(test_data);

        auto upgrp = var.as<updategroup>();
        BOOST_TEST("5jxX" == upgrp.name);

        BOOST_TEST("5jxXg" == upgrp.group.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp.group.key());

        auto root = upgrp.group.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(6 == root.threshold);
        BOOST_TEST(0 == root.weight);

        auto son0 = upgrp.group.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        auto son0_son0 = upgrp.group.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp.group.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        auto son0_son1 = upgrp.group.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp.group.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        auto son1 = upgrp.group.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp.group.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        auto son2 = upgrp.group.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        auto son2_son0 = upgrp.group.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp.group.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        auto son2_son1 = upgrp.group.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp.group.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);

        auto var2   = verify_byte_round_trip_conversion(abis, "updategroup", var);
        auto upgrp2 = var2.as<updategroup>();

        BOOST_TEST("5jxX" == upgrp2.name);

        BOOST_TEST("5jxXg" == upgrp2.group.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp2.group.key());

        root = upgrp2.group.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(6 == root.threshold);
        BOOST_TEST(0 == root.weight);

        son0 = upgrp2.group.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        son0_son0 = upgrp2.group.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp2.group.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        son0_son1 = upgrp2.group.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp2.group.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        son1 = upgrp2.group.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp2.group.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        son2 = upgrp2.group.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        son2_son0 = upgrp2.group.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)upgrp2.group.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        son2_son1 = upgrp2.group.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)upgrp2.group.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);

        verify_type_round_trip_conversion<updategroup>(abis, "updategroup", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newfungible_test) {
    try {
        auto abis = get_evt_abi();

        const char* test_data = R"=====(
        {
          "sym": "5,EVT",
          "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
          "issue" : {
            "name" : "issue",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                "weight": 1
              }
            ]
          },
          "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                "weight": 1
              }
            ]
          },
          "total_supply":"12.00000 EVT"
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto newfg = var.as<newfungible>();

        BOOST_TEST("5,EVT" == newfg.sym.to_string());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newfg.creator);

        BOOST_TEST("issue" == newfg.issue.name);
        BOOST_TEST(1 == newfg.issue.threshold);
        BOOST_TEST_REQUIRE(1 == newfg.issue.authorizers.size());
        BOOST_TEST(newfg.issue.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newfg.issue.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newfg.issue.authorizers[0].weight);

        BOOST_TEST("manage" == newfg.manage.name);
        BOOST_TEST(1 == newfg.manage.threshold);
        BOOST_TEST_REQUIRE(1 == newfg.manage.authorizers.size());
        BOOST_TEST(newfg.manage.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newfg.manage.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newfg.manage.authorizers[0].weight);

        BOOST_TEST(1200000 == newfg.total_supply.get_amount());
        BOOST_TEST("5,EVT" == newfg.total_supply.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == newfg.total_supply.to_string());

        auto var2 = verify_byte_round_trip_conversion(abis, "newfungible", var);

        auto newfg2 = var2.as<newfungible>();

        BOOST_TEST("5,EVT" == newfg2.sym.to_string());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)newfg2.creator);

        BOOST_TEST("issue" == newfg2.issue.name);
        BOOST_TEST(1 == newfg2.issue.threshold);
        BOOST_TEST_REQUIRE(1 == newfg2.issue.authorizers.size());
        BOOST_TEST(newfg2.issue.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newfg2.issue.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newfg2.issue.authorizers[0].weight);

        BOOST_TEST("manage" == newfg2.manage.name);
        BOOST_TEST(1 == newfg2.manage.threshold);
        BOOST_TEST_REQUIRE(1 == newfg2.manage.authorizers.size());
        BOOST_TEST(newfg2.manage.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newfg2.manage.authorizers[0].ref.get_account());
        BOOST_TEST(1 == newfg2.manage.authorizers[0].weight);

        BOOST_TEST(1200000 == newfg2.total_supply.get_amount());
        BOOST_TEST("5,EVT" == newfg2.total_supply.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == newfg2.total_supply.to_string());

        verify_type_round_trip_conversion<newfungible>(abis, "newfungible", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(updfungible_test) {
    try {
        auto abis = get_evt_abi();

        const char* test_data = R"=====(
        {
          "sym": "5,EVT",
          "issue" : {
            "name" : "issue2",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                "weight": 1
              }
            ]
          }
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto updfg = var.as<updfungible>();

        BOOST_TEST("5,EVT" == updfg.sym.to_string());

        BOOST_TEST("issue2" == updfg.issue->name);
        BOOST_TEST(1 == updfg.issue->threshold);
        BOOST_TEST_REQUIRE(1 == updfg.issue->authorizers.size());
        BOOST_TEST(updfg.issue->authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)updfg.issue->authorizers[0].ref.get_account());
        BOOST_TEST(1 == updfg.issue->authorizers[0].weight);

        auto var2 = verify_byte_round_trip_conversion(abis, "updfungible", var);

        auto updfg2 = var2.as<updfungible>();

        BOOST_TEST("5,EVT" == updfg2.sym.to_string());

        BOOST_TEST("issue2" == updfg2.issue->name);
        BOOST_TEST(1 == updfg2.issue->threshold);
        BOOST_TEST_REQUIRE(1 == updfg2.issue->authorizers.size());
        BOOST_TEST(updfg2.issue->authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)updfg2.issue->authorizers[0].ref.get_account());
        BOOST_TEST(1 == updfg2.issue->authorizers[0].weight);

        verify_type_round_trip_conversion<updfungible>(abis, "updfungible", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(issuefungible_test) {
    try {
        auto abis = get_evt_abi();

        const char* test_data = R"=====(
        {
          "address": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "number" : "12.00000 EVT",
          "memo": "memo"
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto issfg = var.as<issuefungible>();

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)issfg.address);
        BOOST_TEST("memo" == issfg.memo);

        BOOST_TEST(1200000 == issfg.number.get_amount());
        BOOST_TEST("5,EVT" == issfg.number.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == issfg.number.to_string());

        auto var2 = verify_byte_round_trip_conversion(abis, "issuefungible", var);

        auto issfg2 = var2.as<issuefungible>();

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)issfg2.address);
        BOOST_TEST("memo" == issfg2.memo);

        BOOST_TEST(1200000 == issfg2.number.get_amount());
        BOOST_TEST("5,EVT" == issfg2.number.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == issfg2.number.to_string());

        verify_type_round_trip_conversion<issuefungible>(abis, "issuefungible", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transferft_test) {
    try {
        auto abis = get_evt_abi();

        const char* test_data = R"=====(
        {
          "from": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "to": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "number" : "12.00000 EVT",
          "memo": "memo"
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto trft = var.as<transferft>();

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)trft.from);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)trft.to);
        BOOST_TEST("memo" == trft.memo);

        BOOST_TEST(1200000 == trft.number.get_amount());
        BOOST_TEST("5,EVT" == trft.number.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == trft.number.to_string());

        auto var2 = verify_byte_round_trip_conversion(abis, "transferft", var);

        auto trft2 = var2.as<transferft>();

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)trft2.from);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)trft2.to);
        BOOST_TEST("memo" == trft2.memo);

        BOOST_TEST(1200000 == trft2.number.get_amount());
        BOOST_TEST("5,EVT" == trft2.number.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == trft2.number.to_string());

        verify_type_round_trip_conversion<transferft>(abis, "transferft", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(addmeta_test) {
    try {
        auto abis = get_evt_abi();

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "key": "key",
          "value": "value",
          "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto admt = var.as<addmeta>();

        BOOST_TEST("key" == admt.key);
        BOOST_TEST("value" == admt.value);
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)admt.creator);

        auto var2 = verify_byte_round_trip_conversion(abis, "addmeta", var);

        auto admt2 = var2.as<addmeta>();

        BOOST_TEST("key" == admt2.key);
        BOOST_TEST("value" == admt2.value);
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)admt2.creator);

        verify_type_round_trip_conversion<addmeta>(abis, "addmeta", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newdelay_test) {
    try {
        auto abis = get_evt_abi();
        BOOST_CHECK(true);
        const char* test_data = R"=======(
        {
            "name": "testdelay",
            "proposer": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "trx": {
                "expiration": "2018-06-30T12:43:24",
                "ref_block_num": 212,
                "ref_block_prefix": 3641962205,
                "delay_sec": 0,
                "actions": [{
                    "name": "newdelay",
                    "domain": "delay",
                    "key": "delay1",
                    "data": "00000000000000000000003090cc053d0002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cfec7a375bd400ddee13d9000100007029b4476e71000000000000000000000000009f077d0000000000000000000000f43150e700e301000000000000000000000000009f077d0002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf000000008052e74c01000000010100000002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf44a2ec000000000100000000b298e982a40100000001020000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000094135c6801000000010100000002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf44a2ec00000000010000"
                }],
                "transaction_extensions": []
            }
        }
        )=======";

        auto var  = fc::json::from_string(test_data);
        auto ndact = var.as<newdelay>();

        BOOST_TEST("testdelay" == (std::string)ndact.name);
        BOOST_TEST(ndact.trx.actions.size() == 1);

        verify_byte_round_trip_conversion(abis, "newdelay", var);
        verify_type_round_trip_conversion<newdelay>(abis, "newdelay", var);
    }
    FC_LOG_AND_RETHROW()
}
