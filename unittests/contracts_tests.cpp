#include <evt/testing/tester.hpp>
#include <evt/chain/token_database.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

extern std::string evt_unittests_dir;

struct contracts_test {
    contracts_test() {
        auto basedir = evt_unittests_dir + "/contracts_tests";
        if(!fc::exists(basedir)) {
            fc::create_directories(basedir);
        }

        controller::config cfg;

        cfg.blocks_dir            = basedir + "blocks";
        cfg.state_dir             = basedir + "state";
        cfg.tokendb_dir           = basedir + "tokendb";
        cfg.state_size            = 1024 * 1024 * 8;
        cfg.reversible_cache_size = 1024 * 1024 * 8;
        cfg.contracts_console     = true;

        cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
        cfg.genesis.initial_key       = tester::get_public_key("evt");
        auto privkey                  = tester::get_private_key("evt");
        my_tester.reset(new tester(cfg));

        my_tester->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        key_seeds.push_back(N(key));

        key = tester::get_public_key(N(key));
        ti  = 0;
    }
    ~contracts_test() {
        my_tester->close();
    }

    const char*
    get_domain_name() {
        static std::string domain_name = "domain" + boost::lexical_cast<std::string>(time(0));
        ;
        return domain_name.c_str();
    }

    const char*
    get_group_name() {
        static std::string group_name = "group" + boost::lexical_cast<std::string>(time(0));
        ;
        return group_name.c_str();
    }

    const char*
    get_delay_name() {
        static std::string delay_name = "delay" + boost::lexical_cast<std::string>(time(0));
        ;
        return delay_name.c_str();
    }

    const char*
    get_symbol_name() {
        static std::string symbol_name;
        if(symbol_name.empty()) {
            srand((unsigned)time(0));
            for(int i = 0; i < 5; i++)
                symbol_name += rand() % 26 + 'A';
        }
        return symbol_name.c_str();
    }

    int32_t
    get_time() {
        return time(0) + (++ti);
    }

    public_key_type           key;
    std::vector<account_name> key_seeds;
    std::unique_ptr<tester>   my_tester;
    int                       ti;
};

BOOST_FIXTURE_TEST_SUITE(contracts_tests, contracts_test)

BOOST_AUTO_TEST_CASE(contract_newdomain_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
            {
              "name" : "domain",
              "creator" : "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
              "issue" : {
                "name" : "issue",
                "threshold" : 1,
                "authorizers": [{
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
                    "weight": 1
                  }
                ]
              }
            }
            )=====";

        auto var    = fc::json::from_string(test_data);
        auto newdom = var.as<newdomain>();

        //newdomain authorization test
        BOOST_CHECK_THROW(my_tester->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), key_seeds), unsatisfied_authorization);

        newdom.creator = key;
        to_variant(newdom, var);
        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), key_seeds), action_authorize_exception);

        newdom.name = get_domain_name();
        newdom.issue.authorizers[0].ref.set_account(key);
        newdom.manage.authorizers[0].ref.set_account(key);

        to_variant(newdom, var);

        my_tester->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), key_seeds);

        //domain_exists_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), key_seeds), tx_duplicate);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_issuetoken_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "domain",
            "names": [
              "t1",
              "t2",
              "t3"
            ],
            "owner": [
              "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2"
            ]
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto istk = var.as<issuetoken>();

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds), action_authorize_exception);

        istk.domain   = get_domain_name();
        istk.owner[0] = key;
        to_variant(istk, var);

        my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds);

        //issue token authorization test
        istk.names = {"r1", "r2", "r3"};
        to_variant(istk, var);
        std::vector<account_name> v2;
        v2.push_back(N(other));
        BOOST_CHECK_THROW(my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), v2), unsatisfied_authorization);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_transfer_test) {
    try {
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

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), key_seeds), action_authorize_exception);

        trf.domain = get_domain_name();
        to_variant(trf, var);

        my_tester->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), key_seeds);

        //transfer token to == from test
        BOOST_CHECK_THROW(my_tester->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), key_seeds), tx_duplicate);
        //
        trf.to[0] = key;
        to_variant(trf, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), key_seeds), unsatisfied_authorization);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_destroytoken_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "cookie",
          "name": "t2"
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto destk = var.as<destroytoken>();

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), key_seeds), action_authorize_exception);

        destk.domain = get_domain_name();
        to_variant(destk, var);

        my_tester->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), key_seeds);

        //destroy token authorization test
        destk.name = "q2";
        to_variant(destk, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), key_seeds), unsatisfied_authorization);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_newgroup_test) {
    try {
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
                  "threshold": 2,
                  "weight": 6,
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
                  "threshold": 2,
                  "weight": 3,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 1
                    }
                  ]
                }
              ]
            }
          }
        }
        )=====";

        auto var = fc::json::from_string(test_data);
        auto gp  = var.as<newgroup>();

        //new group authorization test
        BOOST_CHECK_THROW(my_tester->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds), unsatisfied_authorization);

        gp.group.key_ = key;
        to_variant(gp, var);

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds), action_authorize_exception);

        gp.name        = get_group_name();
        gp.group.name_ = get_group_name();
        to_variant(gp, var);

        my_tester->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds);
        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_updategroup_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "5jxX",
          "group" : {
            "name": "5jxXg",
            "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
            "root": {
              "threshold": 5,
              "weight": 0,
              "nodes": [{
                  "threshold": 2,
                  "weight": 2,
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
                  "weight": 1
                },{
                  "threshold": 2,
                  "weight": 2,
                  "nodes": [{
                      "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                      "weight": 1
                    },{
                      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                      "weight": 1
                    }
                  ]
                }
              ]
            }
          }
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto upgrp = var.as<updategroup>();

        upgrp.group.keys_ = {tester::get_public_key(N(key0)), tester::get_public_key(N(key1)),
                             tester::get_public_key(N(key2)), tester::get_public_key(N(key3)), tester::get_public_key(N(key4))};
        upgrp.name        = get_group_name();
        upgrp.group.name_ = get_group_name();
        to_variant(upgrp, var);
        //new group authorization test
        // BOOST_CHECK_THROW(my_tester->push_action(N(updategroup), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds), unsatisfied_authorization);

        upgrp.group.key_ = key;
        to_variant(upgrp, var);

        my_tester->push_action(N(updategroup), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds);
        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_newfungible_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "sym": "5,EVT",
          "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
          "issue" : {
            "name" : "issue",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] EVT6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
                "weight": 1
              }
            ]
          },
          "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
                "weight": 1
              }
            ]
          },
          "total_supply":"15.00000 EVT"
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto newfg = var.as<newfungible>();

        newfg.sym          = symbol::from_string(string("5,") + get_symbol_name());
        newfg.total_supply = asset::from_string(string("15.00000 ") + get_symbol_name());
        to_variant(newfg, var);
        //new fungible authorization test
        BOOST_CHECK_THROW(my_tester->push_action(N(newfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), unsatisfied_authorization);

        newfg.creator = key;
        to_variant(newfg, var);

        my_tester->push_action(N(newfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds);
        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_updfungible_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "sym": "5,EVT",
          "issue" : {
            "name" : "issue",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] EVT6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
                "weight": 2
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

        auto var   = fc::json::from_string(test_data);
        auto updfg = var.as<updfungible>();

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(updfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), action_authorize_exception);

        updfg.sym = symbol::from_string(string("5,") + get_symbol_name());
        updfg.manage->authorizers[0].ref.set_account(key);
        to_variant(updfg, var);

        my_tester->push_action(N(updfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_issuefungible_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "address": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "number" : "12.00000 EVT",
          "memo": "memo"
        }
        )=====";

        auto var     = fc::json::from_string(test_data);
        auto issfg   = var.as<issuefungible>();
        issfg.number = asset::from_string(string("150.00000 ") + get_symbol_name());
        to_variant(issfg, var);

        //issue rft more than balance exception
        BOOST_CHECK_THROW(my_tester->push_action(N(issuefungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), fungible_supply_exception);

        issfg.number  = asset::from_string(string("15.00000 ") + get_symbol_name());
        issfg.address = key;
        to_variant(issfg, var);
        my_tester->push_action(N(issuefungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds);

        to_variant(issfg, var);
        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_transferft_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "from": "EVT6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
          "to": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
          "number" : "12.00000 EVT",
          "memo": "memo"
        }
        )=====";

        auto var    = fc::json::from_string(test_data);
        auto trft   = var.as<transferft>();
        trft.number = asset::from_string(string("150.00000 ") + get_symbol_name());
        to_variant(trft, var);

        //transfer rft more than balance exception
        BOOST_CHECK_THROW(my_tester->push_action(N(transferft), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), balance_exception);

        trft.number = asset::from_string(string("15.00000 ") + get_symbol_name());
        to_variant(trft, var);
        my_tester->push_action(N(transferft), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_updatedomain_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "name" : "domain",
          "issue" : {
            "name": "issue",
            "threshold": 2,
            "authorizers": [{
              "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
              "weight": 2
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
                "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
                "weight": 1
              }
            ]
          }
        }
        )=====";

        auto var   = fc::json::from_string(test_data);
        auto updom = var.as<updatedomain>();

        //action_authorize_exception test
        BOOST_CHECK_THROW(my_tester->push_action(N(updatedomain), string_to_name128(get_domain_name()), N128(.update), var.get_object(), key_seeds), action_authorize_exception);

        updom.name = get_domain_name();
        updom.issue->authorizers[0].ref.set_group(get_group_name());
        updom.manage->authorizers[0].ref.set_account(key);
        to_variant(updom, var);

        my_tester->push_action(N(updatedomain), string_to_name128(get_domain_name()), N128(.update), var.get_object(), key_seeds);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_group_auth_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "domain": "domain",
            "names": [
              "authorizers1",
            ],
            "owner": [
              "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2"
            ]
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto istk = var.as<issuetoken>();

        istk.domain   = get_domain_name();
        istk.owner[0] = key;
        to_variant(istk, var);

        std::vector<account_name> seeds1 = {N(key0), N(key1), N(key2), N(key3)};
        BOOST_CHECK_THROW(my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), seeds1), unsatisfied_authorization);

        istk.names[0] = "authorizers2";
        to_variant(istk, var);
        std::vector<account_name> seeds2 = {N(key1), N(key2), N(key3), N(key4)};
        BOOST_CHECK_THROW(my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), seeds2), unsatisfied_authorization);

        istk.names[0] = "authorizers3";
        to_variant(istk, var);
        std::vector<account_name> seeds3 = {N(key0), N(key1), N(key2), N(key3), N(key4)};
        my_tester->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), seeds3);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_addmeta_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
        {
          "key": "key",
          "value": "value",
          "creator": "[A] EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
        }
        )=====";

        auto var  = fc::json::from_string(test_data);
        auto admt = var.as<addmeta>();

        //meta authorizers test
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), string_to_name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds), unsatisfied_authorization);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds), unsatisfied_authorization);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), unsatisfied_authorization);

        //meta authorizers test
        admt.creator = tester::get_public_key(N(other));
        to_variant(admt, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), string_to_name128(get_domain_name()), N128(.meta), var.get_object(), {N(other)}), meta_involve_exception);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(group), string_to_name128(get_group_name()), var.get_object(), {N(other)}), meta_involve_exception);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), {N(other)}), meta_involve_exception);

        admt.creator = key;
        to_variant(admt, var);

        my_tester->push_action(N(addmeta), string_to_name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds);

        my_tester->push_action(N(addmeta), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds);

        my_tester->push_action(N(addmeta), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds);

        //meta_key_exception test
        admt.value = "value2";
        to_variant(admt, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), string_to_name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds), meta_key_exception);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(group), string_to_name128(get_group_name()), var.get_object(), key_seeds), meta_key_exception);
        BOOST_CHECK_THROW(my_tester->push_action(N(addmeta), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), key_seeds), meta_key_exception);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_faildelay_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=======(
        {
            "name": "testdelay",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "trx": {
                "expiration": "2021-07-04T05:14:12",
                "ref_block_num": "3432",
                "ref_block_prefix": "291678901",
                "actions": [
                ],
                "transaction_extensions": []
            }
        }
        )=======";

        auto var  = fc::json::from_string(test_data);
        auto ndact = var.as<newdelay>();
        ndact.name = get_delay_name();

        const char* newdomain_test_data = R"=====(
            {
              "name" : "domain",
              "creator" : "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
              "issue" : {
                "name" : "issue",
                "threshold" : 1,
                "authorizers": [{
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
                    "weight": 1
                  }
                ]
              }
            }
            )=====";

        auto newdomain_var    = fc::json::from_string(newdomain_test_data);
        auto newdom = newdomain_var.as<newdomain>();
        newdom.creator = tester::get_public_key(N(delay_key));
        to_variant(newdom, newdomain_var);
        ndact.trx.actions.push_back(my_tester->get_action(N(newdomain), N128(domain), N128(.create), newdomain_var.get_object()));

        to_variant(ndact, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(newdelay), N128(delay), string_to_name128(get_delay_name()), var.get_object(), key_seeds), unsatisfied_authorization);

        ndact.proposer = key;
        to_variant(ndact, var);

        my_tester->push_action(N(newdelay), N128(delay),  string_to_name128(get_delay_name()), var.get_object(), key_seeds);

        const char* execute_test_data = R"=======(
        {
            "name": "testdelay",
            "executor": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3"
        }
        )=======";

        auto execute_tvar  = fc::json::from_string(execute_test_data);
        auto edact = execute_tvar.as<executedelay>();
        edact.executor = key;
        edact.name = get_delay_name();
        to_variant(edact, execute_tvar);

        //delay_executor_exception
        BOOST_CHECK_THROW(my_tester->push_action(N(executedelay), N128(delay), string_to_name128(get_delay_name()), execute_tvar.get_object(), {N(key)}),delay_executor_exception);

        auto& tokendb = my_tester->control->token_db();
        delay_def delay;
        tokendb.read_delay(edact.name, delay);
        BOOST_TEST(delay.status == delay_status::proposed);

        auto sig= tester::get_private_key(N(delay_key)).sign(delay.trx.sig_digest(my_tester->control->get_chain_id()));
        auto sig2= tester::get_private_key(N(key)).sign(delay.trx.sig_digest(my_tester->control->get_chain_id()));
        const char* approve_test_data = R"=======(
        {
            "name": "testdelay",
            "signatures": [
            ]
        }
        )=======";

        auto approve_var  = fc::json::from_string(approve_test_data);
        auto adact = approve_var.as<approvedelay>();
        adact.name = get_delay_name();
        adact.signatures = {sig,sig2};
        to_variant(adact, approve_var);

        BOOST_CHECK_THROW(my_tester->push_action(N(approvedelay), N128(delay),  string_to_name128(get_delay_name()), approve_var.get_object(), key_seeds),delay_not_required_keys_exception);

        tokendb.read_delay(edact.name, delay);
        BOOST_TEST(delay.status == delay_status::proposed);

                const char* cancel_test_data = R"=======(
        {
            "name": "testdelay"
        }
        )=======";
        auto cancel_var  = fc::json::from_string(test_data);
        auto cdact = var.as<canceldelay>();
        cdact.name = get_delay_name();
        to_variant(cdact, cancel_var);

        my_tester->push_action(N(canceldelay), N128(delay), string_to_name128(get_delay_name()), cancel_var.get_object(), key_seeds);

        tokendb.read_delay(edact.name, delay);
        BOOST_TEST(delay.status == delay_status::cancelled);

        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(contract_successdelay_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=======(
        {
            "name": "testdelay",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "trx": {
                "expiration": "2021-07-04T05:14:12",
                "ref_block_num": "3432",
                "ref_block_prefix": "291678901",
                "actions": [
                ],
                "transaction_extensions": []
            }
        }
        )=======";

        auto var  = fc::json::from_string(test_data);
        auto ndact = var.as<newdelay>();

        const char* newdomain_test_data = R"=====(
            {
              "name" : "domain",
              "creator" : "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
              "issue" : {
                "name" : "issue",
                "threshold" : 1,
                "authorizers": [{
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
                    "ref": "[A] EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
                    "weight": 1
                  }
                ]
              }
            }
            )=====";

        auto newdomain_var    = fc::json::from_string(newdomain_test_data);
        auto newdom = newdomain_var.as<newdomain>();
        newdom.creator = tester::get_public_key(N(delay_key));
        to_variant(newdom, newdomain_var);
        ndact.trx.actions.push_back(my_tester->get_action(N(newdomain), N128(domain), N128(.create), newdomain_var.get_object()));

        to_variant(ndact, var);
        BOOST_CHECK_THROW(my_tester->push_action(N(newdelay), N128(delay), N128(testdelay), var.get_object(), key_seeds), unsatisfied_authorization);

        ndact.proposer = key;
        to_variant(ndact, var);

        my_tester->push_action(N(newdelay), N128(delay), N128(testdelay), var.get_object(), key_seeds);

        

        auto& tokendb = my_tester->control->token_db();
        delay_def delay;
        tokendb.read_delay(ndact.name, delay);
        BOOST_TEST(delay.status == delay_status::proposed);

        auto sig= tester::get_private_key(N(delay_key)).sign(delay.trx.sig_digest(my_tester->control->get_chain_id()));
        auto sig2= tester::get_private_key(N(key)).sign(delay.trx.sig_digest(my_tester->control->get_chain_id()));
        const char* approve_test_data = R"=======(
        {
            "name": "testdelay",
            "signatures": [
            ]
        }
        )=======";

        auto approve_var  = fc::json::from_string(approve_test_data);
        auto adact = approve_var.as<approvedelay>();
        adact.signatures = {sig};
        to_variant(adact, approve_var);

        my_tester->push_action(N(approvedelay), N128(delay),  N128(testdelay), approve_var.get_object(), {});

        tokendb.read_delay(adact.name, delay);
        BOOST_TEST(delay.status == delay_status::proposed);

        const char* execute_test_data = R"=======(
        {
            "name": "testdelay",
            "executor": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3"
        }
        )=======";

        auto execute_tvar  = fc::json::from_string(execute_test_data);
        auto edact = execute_tvar.as<executedelay>();
        edact.executor = tester::get_public_key(N(delay_key));
        to_variant(edact, execute_tvar);

        my_tester->push_action(N(executedelay), N128(delay), N128(testdelay), execute_tvar.get_object(), {N(delay_key)});

        tokendb.read_delay(edact.name, delay);
        BOOST_TEST(delay.status == delay_status::executed);


        my_tester->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()