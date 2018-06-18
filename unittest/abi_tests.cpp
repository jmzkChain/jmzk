#define BOOST_TEST_MODULE unittests
#define BOOST_TEST_DYN_LINK

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <fc/variant.hpp>

#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/types.hpp>
// #include <evt/abi_generator/abi_generator.hpp>

#include <boost/test/framework.hpp>

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
get_resolver(const abi_def& abi = abi_def()) {
    return [&abi](const account_name& name) -> optional<abi_serializer> {
        return abi_serializer(evt_contract_abi());
    };
}

// verify that round trip conversion, via actual class, reproduces the exact same data
template <typename T>
fc::variant
verify_type_round_trip_conversion(const abi_serializer& abis, const type_name& type, const fc::variant& var) {
    try {
        auto bytes = abis.variant_to_binary(type, var);

        T obj;
        abi_serializer::from_variant(var, obj, get_resolver());

        fc::variant var2;
        abi_serializer::to_variant(obj, var2, get_resolver());

        std::string r = fc::json::to_string(var2);

        auto bytes2 = abis.variant_to_binary(type, var2);

        BOOST_TEST(fc::to_hex(bytes) == fc::to_hex(bytes2));

        return var2;
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newdomain_test) {
    try {
        abi_serializer abis(evt_contract_abi());

        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "cookie",
          "issuer" : "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
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
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)newdom.issuer);

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
        // BOOST_TEST("OWNER" == (std::string)newdom.issue.authorizers[0].ref.get_group());
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
        BOOST_TEST((std::string)newdom2.issuer == (std::string)newdom.issuer);

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
        abi_serializer abis(evt_contract_abi());

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
          "transfer":{},
          "manage":{}
        }
        )=====";

        auto var    = fc::json::from_string(test_data);
        auto updom = var.as<updatedomain>();

        BOOST_TEST("cookie" == updom.name);

        BOOST_TEST("issue" == updom.issue->name);
        BOOST_TEST(2 == updom.issue->threshold);
        BOOST_TEST_REQUIRE(2 == updom.issue->authorizers.size());
        BOOST_TEST_REQUIRE(updom.issue->authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)updom.issue->authorizers[0].ref.get_account());
        BOOST_TEST(1 == updom.issue->authorizers[0].weight);

        auto var2    = verify_byte_round_trip_conversion(abis, "updatedomain", var);
        // auto updom2 = var2.as<updatedomain>();

        // BOOST_TEST("cookie" == updom2.name);

        // BOOST_TEST("issue" == updom2.issue->name);
        // BOOST_TEST(2 == updom2.issue->threshold);
        // BOOST_TEST_REQUIRE(2 == updom2.issue->authorizers.size());
        // BOOST_TEST_REQUIRE(updom2.issue->authorizers[0].ref.is_account_ref());
        // BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)updom2.issue->authorizers[0].ref.get_account());
        // BOOST_TEST(1 == updom2.issue->authorizers[0].weight);

        // verify_type_round_trip_conversion<updatedomain>(abis, "updatedomain", var);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(newgroup_test) {
    try {
        abi_serializer abis(evt_contract_abi());

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
