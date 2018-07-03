#include <evt/testing/tester.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

struct contracts_test {
    contracts_test() {
        controller::config cfg;
        cfg.blocks_dir            = "/tmp/evt_unittests/blocks59";   //strcat("/tmp/evt_unittests/", chain::config::default_blocks_dir_name);
        cfg.state_dir             = "/tmp/evt_unittests/state59";    //strcat("/tmp/evt_unittests/" , chain::config::default_state_dir_name);
        cfg.tokendb_dir           = "/tmp/evt_unittests/tokendb59";  //strcat("/tmp/evt_unittests/" , chain::config::default_tokendb_dir_name);
        cfg.state_size            = 1024 * 1024 * 8;
        cfg.reversible_cache_size = 1024 * 1024 * 8;
        cfg.contracts_console     = true;

        cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
        cfg.genesis.initial_key       = tester::get_public_key("evt");
        auto privkey                  = tester::get_private_key("evt");
        ter.reset(new tester(cfg));

        ter->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        v.push_back(N(key));

        key = tester::get_public_key(N(key));
        ti  = 0;
    }
    ~contracts_test() {
        ter->close();
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
    std::vector<account_name> v;
    std::unique_ptr<tester>   ter;
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
        BOOST_CHECK_THROW(ter->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), v), unsatisfied_authorization);

        newdom.creator = key;
        to_variant(newdom, var);
        //action_authorize_exception test
        BOOST_CHECK_THROW(ter->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), v), action_authorize_exception);

        newdom.name = get_domain_name();
        newdom.issue.authorizers[0].ref.set_account(key);
        newdom.manage.authorizers[0].ref.set_account(key);

        to_variant(newdom, var);

        ter->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), v);

        //domain_exists_exception test
        BOOST_CHECK_THROW(ter->push_action(N(newdomain), string_to_name128(get_domain_name()), N128(.create), var.get_object(), v), tx_duplicate);

        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(updatedomain), string_to_name128(get_domain_name()), N128(.update), var.get_object(), v), action_authorize_exception);

        updom.name = get_domain_name();
        updom.issue->authorizers[0].ref.set_account(key);
        to_variant(updom, var);

        ter->push_action(N(updatedomain), string_to_name128(get_domain_name()), N128(.update), var.get_object(), v);

        //updatedomian authorization test
        updom.manage->authorizers[0].ref.set_account(key);
        to_variant(updom, var);
        BOOST_CHECK_THROW(ter->push_action(N(updatedomain), string_to_name128(get_domain_name()), N128(.update), var.get_object(), v), unsatisfied_authorization);

        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), v), action_authorize_exception);

        istk.domain   = get_domain_name();
        istk.owner[0] = key;
        to_variant(istk, var);

        ter->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), v);

        //issue token authorization test
        istk.names = {"r1", "r2", "r3"};
        to_variant(istk, var);
        std::vector<account_name> v2;
        v2.push_back(N(other));
        BOOST_CHECK_THROW(ter->push_action(N(issuetoken), string_to_name128(get_domain_name()), N128(.issue), var.get_object(), v2), unsatisfied_authorization);

        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), v), action_authorize_exception);

        trf.domain = get_domain_name();
        to_variant(trf, var);

        ter->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), v);

        //transfer token to == from test
        BOOST_CHECK_THROW(ter->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), v), tx_duplicate);
        //
        trf.to[0] = key;
        to_variant(trf, var);
        BOOST_CHECK_THROW(ter->push_action(N(transfer), string_to_name128(get_domain_name()), N128(t1), var.get_object(), v), unsatisfied_authorization);

        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), v), action_authorize_exception);

        destk.domain = get_domain_name();
        to_variant(destk, var);

        ter->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), v);

        //destroy token authorization test
        destk.name = "q2";
        to_variant(destk, var);
        BOOST_CHECK_THROW(ter->push_action(N(destroytoken), string_to_name128(get_domain_name()), N128(t2), var.get_object(), v), unsatisfied_authorization);

        ter->produce_blocks();
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

        auto var       = fc::json::from_string(test_data);
        auto gp        = var.as<newgroup>();

        //new group authorization test
        BOOST_CHECK_THROW(ter->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), v), unsatisfied_authorization);

        gp.group.key_  = key;
        to_variant(gp, var);

        //action_authorize_exception test
        BOOST_CHECK_THROW(ter->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), v), action_authorize_exception);

        gp.name        = get_group_name();
        gp.group.name_ = get_group_name();
        to_variant(gp, var);

        ter->push_action(N(newgroup), N128(group), string_to_name128(get_group_name()), var.get_object(), v);
        ter->produce_blocks();
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

        auto var          = fc::json::from_string(test_data);
        auto upgrp        = var.as<updategroup>();

        upgrp.name        = get_group_name();
        upgrp.group.name_ = get_group_name();
        to_variant(upgrp, var);
        //new group authorization test
        // BOOST_CHECK_THROW(ter->push_action(N(updategroup), N128(group), string_to_name128(get_group_name()), var.get_object(), v), unsatisfied_authorization);

        upgrp.group.key_  = key;
        to_variant(upgrp, var);

        ter->push_action(N(updategroup), N128(group), string_to_name128(get_group_name()), var.get_object(), v);
        ter->produce_blocks();
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

        auto var           = fc::json::from_string(test_data);
        auto newfg         = var.as<newfungible>();

        newfg.sym          = symbol::from_string(string("5,") + get_symbol_name());
        newfg.total_supply = asset::from_string(string("15.00000 ") + get_symbol_name());
        to_variant(newfg, var);
        //new fungible authorization test
        BOOST_CHECK_THROW(ter->push_action(N(newfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v), unsatisfied_authorization);

        newfg.creator      = key;
        to_variant(newfg, var);

        ter->push_action(N(newfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v);
        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(updfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v), action_authorize_exception);

        updfg.sym  = symbol::from_string(string("5,") + get_symbol_name());
        to_variant(updfg, var);

        ter->push_action(N(updfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v);

        //updfungible authorization test
        updfg.manage->authorizers[0].ref.set_account(key);
        to_variant(updfg, var);
        BOOST_CHECK_THROW(ter->push_action(N(updfungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v), unsatisfied_authorization);

        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(issuefungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v),fungible_supply_exception);

        issfg.number = asset::from_string(string("15.00000 ") + get_symbol_name());
        issfg.address = key;
        to_variant(issfg, var);
        ter->push_action(N(issuefungible), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v);

        to_variant(issfg, var);
        ter->produce_blocks();
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
        BOOST_CHECK_THROW(ter->push_action(N(transferft), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v),balance_exception);

        trft.number = asset::from_string(string("15.00000 ") + get_symbol_name());
        to_variant(trft, var);
        ter->push_action(N(transferft), N128(fungible), string_to_name128(get_symbol_name()), var.get_object(), v);

        ter->produce_blocks();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()