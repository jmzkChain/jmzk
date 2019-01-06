#include <catch/catch.hpp>

#include <evt/chain/token_database.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>
#include <evt/testing/tester.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

extern std::string evt_unittests_dir;

class contracts_test {
public:
    contracts_test() {
        auto basedir = evt_unittests_dir + "/contracts_tests";
        if(!fc::exists(basedir)) {
            fc::create_directories(basedir);
        }

        auto cfg = controller::config();

        cfg.blocks_dir            = basedir + "blocks";
        cfg.state_dir             = basedir + "state";
        cfg.tokendb_dir           = basedir + "tokendb";
        cfg.contracts_console     = true;
        cfg.charge_free_mode      = false;
        cfg.loadtest_mode         = false;

        cfg.genesis.initial_timestamp = fc::time_point::now();
        cfg.genesis.initial_key       = tester::get_public_key("evt");
        auto privkey                  = tester::get_private_key("evt");
        my_tester.reset(new tester(cfg));

        my_tester->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        key_seeds.push_back(N(key));
        key_seeds.push_back("evt");
        key_seeds.push_back("evt2");
        key_seeds.push_back(N(payer));
        key_seeds.push_back(N(poorer));

        key         = tester::get_public_key(N(key));
        private_key = tester::get_private_key(N(key));
        payer       = address(tester::get_public_key(N(payer)));
        poorer      = address(tester::get_public_key(N(poorer)));

        my_tester->add_money(payer, asset(1'000'000'000'000, symbol(5, EVT_SYM_ID)));

        ti = 0;
    }

    ~contracts_test() {
        my_tester->close();
    }

protected:
    std::string
    get_domain_name(int seq = 0) {
        static auto base_time = time(0);

        auto name = std::string("domain");
        name.append(std::to_string(base_time + seq));
        return name;
    }

    const char*
    get_group_name() {
        static std::string group_name = "group" + boost::lexical_cast<std::string>(time(0));
        return group_name.c_str();
    }

    const char*
    get_suspend_name() {
        static std::string suspend_name = "suspend" + boost::lexical_cast<std::string>(time(0));
        return suspend_name.c_str();
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
    
    symbol_id_type
    get_sym_id() {
        auto sym_id = 3;

        return sym_id;
    }

    int32_t
    get_time() {
        return time(0) + (++ti);
    }

protected:
    public_key_type           key;
    private_key_type          private_key;
    address                   payer;
    address                   poorer;
    std::vector<account_name> key_seeds;
    std::unique_ptr<tester>   my_tester;
    int                       ti;
    symbol_id_type            sym_id;
};

TEST_CASE_METHOD(contracts_test, "contract_newdomain_test", "[contracts]") {
    CHECK(true);
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
                "ref": "[G] .OWNER",
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

    auto  var     = fc::json::from_string(test_data);
    auto  newdom  = var.as<newdomain>();
    auto& tokendb = my_tester->control->token_db();

    CHECK(!tokendb.exists_domain(get_domain_name()));

    //newdomain authorization test
    CHECK_THROWS_AS(my_tester->push_action(N(newdomain), name128(get_domain_name()), N128(.create), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    newdom.creator = key;
    to_variant(newdom, var);
    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(newdomain), name128(get_domain_name()), N128(.create), var.get_object(), key_seeds, payer), action_authorize_exception);

    newdom.name = ".domains";
    to_variant(newdom, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newdomain), name128(".domains"), N128(.create), var.get_object(), key_seeds, payer), name_reserved_exception);

    newdom.name = get_domain_name();
    newdom.issue.authorizers[0].ref.set_account(key);
    newdom.manage.authorizers[0].ref.set_account(key);

    to_variant(newdom, var);

    my_tester->push_action(N(newdomain), name128(get_domain_name()), N128(.create), var.get_object(), key_seeds, payer);

    //domain_duplicate_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(newdomain), name128(get_domain_name()), N128(.create), var.get_object(), key_seeds, payer), tx_duplicate);

    CHECK(tokendb.exists_domain(get_domain_name()));

    newdom.name = get_domain_name(1);
    my_tester->push_action(action(newdom.name, N128(.create), newdom), key_seeds, payer);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_issuetoken_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "domain": "domain",
        "names": [
          "t1",
          "t2",
          "t3",
          "t4"
        ],
        "owner": [
          "EVT5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2"
        ]
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  istk    = var.as<issuetoken>();
    auto& tokendb = my_tester->control->token_db();

    CHECK(!tokendb.exists_token(get_domain_name(), "t1"));

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, payer), action_authorize_exception);

    istk.domain   = get_domain_name();
    istk.owner[0] = key;
    to_variant(istk, var);

    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, address(N(.domain), name128(get_domain_name()), 0)), charge_exceeded_exception);

    my_tester->add_money(address(N(.domain), name128(get_domain_name()), 0), asset(10'000'000, symbol(5, EVT_SYM_ID)));
    my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, address(N(.domain), name128(get_domain_name()), 0));

    istk.domain = get_domain_name(1);
    my_tester->push_action(action(get_domain_name(1), N128(.issue), istk), key_seeds, payer);

    istk.domain = get_domain_name();
    istk.names  = {".t1", ".t2", ".t3"};
    to_variant(istk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, payer), name_reserved_exception);

    istk.names = {"r1", "r2", "r3"};
    istk.owner.clear();
    to_variant(istk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, payer), token_owner_exception);

    istk.owner.emplace_back(address());
    to_variant(istk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, payer), address_reserved_exception);

    istk.owner[0].set_generated(".abc", "test", 0);
    to_variant(istk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), key_seeds, payer), address_reserved_exception);

    //issue token authorization test
    istk.owner[0] = key;
    istk.names = {"r1", "r2", "r3"};
    to_variant(istk, var);
    std::vector<account_name> v2;
    v2.push_back(N(other));
    v2.push_back(N(payer));
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), v2, payer), unsatisfied_authorization);

    CHECK(tokendb.exists_token(get_domain_name(), "t1"));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_transfer_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "domain": "cookie",
      "name": "t1",
      "to": [
        "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
      ],
      "memo":"memo"
    }
    )=====";
    auto&       tokendb   = my_tester->control->token_db();
    token_def   tk;
    tokendb.read_token(get_domain_name(), "t1", tk);
    CHECK(1 == tk.owner.size());

    auto var = fc::json::from_string(test_data);
    auto trf = var.as<transfer>();

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), action_authorize_exception);

    trf.domain = get_domain_name();
    trf.to.clear();
    to_variant(trf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), token_owner_exception);

    trf.to.emplace_back(address());
    to_variant(trf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), address_reserved_exception);

    trf.to[0].set_generated(".abc", "test", 0);
    to_variant(trf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), address_reserved_exception);

    trf.to.clear();
    trf.to.emplace_back("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX");
    trf.to.emplace_back("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV");
    to_variant(trf, var);
    my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer);

    tokendb.read_token(get_domain_name(), "t1", tk);
    CHECK(2 == tk.owner.size());

    trf.to[1] = key;
    to_variant(trf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_destroytoken_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "domain": "cookie",
      "name": "t2"
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  destk   = var.as<destroytoken>();
    auto& tokendb = my_tester->control->token_db();

    CHECK(tokendb.exists_token(get_domain_name(), "t2"));

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(destroytoken), name128(get_domain_name()), N128(t2), var.get_object(), key_seeds, payer), action_authorize_exception);

    destk.domain = get_domain_name();
    to_variant(destk, var);

    my_tester->push_action(N(destroytoken), name128(get_domain_name()), N128(t2), var.get_object(), key_seeds, payer);

    //destroy token authorization test
    destk.name = "q2";
    to_variant(destk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(destroytoken), name128(get_domain_name()), N128(t2), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    token_def tk;
    tokendb.read_token(get_domain_name(), "t2", tk);
    CHECK(address() == tk.owner[0]);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_destroytoken_auth_test", "[contracts]") {
    auto am    = addmeta();
    am.key     = N128(.invalid-key);
    am.value   = "invalid-value";
    am.creator = key; 

    // meta key is not valid
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(.meta), am), key_seeds, payer, 5'000'000), meta_key_exception);

    am.key = N128(.disable-destroy);
    // `.distable-destroy` meta key cannot be added to token
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), am), key_seeds, payer, 5'000'000), meta_key_exception);
    // value for `.distable-destroy` is not valid, only 'true' or 'false' is valid
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(.meta), am), key_seeds, payer, 5'000'000), meta_value_exception);

    am.value = "false";
    // add `.distable-destroy` with 'false' to domain-0
    my_tester->push_action(action(get_domain_name(), N128(.meta), am), key_seeds, payer, 5'000'000);

    am.value = "true";
    // add `.distable-destroy` with 'true' to domain-1
    my_tester->push_action(action(get_domain_name(1), N128(.meta), am), key_seeds, payer, 5'000'000);

    auto dt   = destroytoken();
    dt.domain = get_domain_name();
    dt.name   = N128(t4);

    // value of `.distable-destroy` is 'false', can destroy
    CHECK_NOTHROW(my_tester->push_action(action(dt.domain, dt.name, dt), key_seeds, payer));

    dt.domain = get_domain_name(1);
    // value of `.distable-destroy` is 'true', cannot destroy
    CHECK_THROWS_AS(my_tester->push_action(action(dt.domain, dt.name, dt), key_seeds, payer), token_cannot_destroy_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_newgroup_test", "[contracts]") {
    CHECK(true);
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

    auto  var         = fc::json::from_string(test_data);
    auto  group_payer = address(N(.domain), ".group", 0);
    auto& tokendb     = my_tester->control->token_db();

    CHECK(!tokendb.exists_group(get_group_name()));
    my_tester->add_money(group_payer, asset(10'000'000, symbol(5, EVT_SYM_ID)));

    auto gp = var.as<newgroup>();

    //new group authorization test
    CHECK_THROWS_AS(my_tester->push_action(N(newgroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, group_payer), unsatisfied_authorization);

    gp.group.key_ = key;
    to_variant(gp, var);

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(newgroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, group_payer), action_authorize_exception);

    gp.name = "xxx";
    to_variant(gp, var);

    //name match test
    CHECK_THROWS_AS(my_tester->push_action(N(newgroup), N128(.group), name128("xxx"), var.get_object(), key_seeds, group_payer), group_name_exception);

    gp.name        = get_group_name();
    gp.group.name_ = "sdf";
    to_variant(gp, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newgroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, group_payer), group_name_exception);

    gp.group.name_ = get_group_name();
    to_variant(gp, var);
    my_tester->push_action(N(newgroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, group_payer);

    gp.name        = ".gp";
    gp.group.name_ = ".gp";
    to_variant(gp, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newgroup), N128(.group), name128(".gp"), var.get_object(), key_seeds, group_payer), name_reserved_exception);

    CHECK(tokendb.exists_group(get_group_name()));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_updategroup_test", "[contracts]") {
    CHECK(true);
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

    auto  var     = fc::json::from_string(test_data);
    auto  upgrp   = var.as<updategroup>();
    auto& tokendb = my_tester->control->token_db();

    CHECK(tokendb.exists_group(get_group_name()));
    group gp;
    tokendb.read_group(get_group_name(), gp);
    CHECK(6 == gp.root().threshold);

    upgrp.group.keys_ = {tester::get_public_key(N(key0)), tester::get_public_key(N(key1)),
                         tester::get_public_key(N(key2)), tester::get_public_key(N(key3)), tester::get_public_key(N(key4))};
    
    to_variant(upgrp, var);


    CHECK_THROWS_AS(my_tester->push_action(N(updategroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer), action_authorize_exception);

    //updategroup group authorization test
    upgrp.name        = get_group_name();
    upgrp.group.name_ = get_group_name();
    to_variant(upgrp, var);
    // CHECK_THROWS_AS(my_tester->push_action(N(updategroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer),unsatisfied_authorization);

    upgrp.name        = get_group_name();
    upgrp.group.name_ = get_group_name();
    upgrp.group.key_  = key;
    to_variant(upgrp, var);
    my_tester->push_action(N(updategroup), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer);

    tokendb.read_group(get_group_name(), gp);
    CHECK(5 == gp.root().threshold);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_newfungible_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "name": "EVT",
      "sym_name": "EVT",
      "sym": "5,S#3",
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
      "total_supply":"100.00000 S#3"
    }
    )=====";

    auto var            = fc::json::from_string(test_data);
    auto fungible_payer = address(N(.domain), ".fungible", 0);
    my_tester->add_money(fungible_payer, asset(10'000'000, symbol(5, EVT_SYM_ID)));
    auto& tokendb = my_tester->control->token_db();

    CHECK(!tokendb.exists_fungible(3));

    auto newfg = var.as<newfungible>();

    newfg.name          = get_symbol_name();
    newfg.sym_name      = get_symbol_name();
    newfg.total_supply = asset::from_string(string("100.00000 S#3"));
    to_variant(newfg, var);
    //new fungible authorization test
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer), unsatisfied_authorization);

    newfg.creator = key;
    newfg.issue.authorizers[0].ref.set_account(key);
    newfg.manage.authorizers[0].ref.set_account(key);
    to_variant(newfg, var);
    my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer);

    newfg.name          = "lala";
    newfg.sym_name      = "lala";
    newfg.total_supply = asset::from_string(string("10.00000 S#3"));
    to_variant(newfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer),fungible_duplicate_exception);

    newfg.total_supply = asset::from_string(string("0.00000 S#3"));
    to_variant(newfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer), fungible_supply_exception);

    CHECK(tokendb.exists_fungible(get_sym_id()));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_updfungible_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "sym_id": "0",
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

    auto  var     = fc::json::from_string(test_data);
    auto  updfg   = var.as<updfungible>();
    auto& tokendb = my_tester->control->token_db();

    fungible_def fg;
    tokendb.read_fungible(get_sym_id(), fg);
    CHECK(1 == fg.issue.authorizers[0].weight);

    //action_authorize_exception test
    auto strkey = (std::string)key;
    CHECK_THROWS_AS(my_tester->push_action(N(updfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), action_authorize_exception);

    updfg.sym_id = get_sym_id();
    updfg.issue->authorizers[0].ref.set_account(key);
    updfg.manage->authorizers[0].ref.set_account(tester::get_public_key(N(key2)));
    to_variant(updfg, var);

    my_tester->push_action(N(updfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    tokendb.read_fungible(get_sym_id(), fg);
    CHECK(2 == fg.issue.authorizers[0].weight);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_issuefungible_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "address": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "number" : "12.00000 S#1",
      "memo": "memo"
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  issfg   = var.as<issuefungible>();
    auto& tokendb = my_tester->control->token_db();
    CHECK(!tokendb.exists_asset(key, symbol(5, get_sym_id())));

    issfg.number = asset::from_string(string("150.00000 S#") + std::to_string(get_sym_id()));
    to_variant(issfg, var);

    //issue rft more than balance exception
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), fungible_supply_exception);

    issfg.number  = asset::from_string(string("50.00000 S#") + std::to_string(get_sym_id()));
    issfg.address = key;
    to_variant(issfg, var);

    issfg.address.set_reserved();
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    issfg.address.set_generated(".abc", "test", 123);
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    issfg.address = key;
    to_variant(issfg, var); 
    my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    issfg.number = asset::from_string(string("15.00000 S#0"));
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible),(name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), action_authorize_exception);

    asset ast;
    tokendb.read_asset(key, symbol(5, get_sym_id()), ast);
    CHECK(5000000 == ast.amount());

    issfg.number = asset::from_string(string("15.00000 S#1"));
    to_variant(issfg, var);

    signed_transaction trx;
    trx.actions.emplace_back(my_tester->get_action(N(issuefungible), N128(.fungible), (name128)std::to_string(1), var.get_object()));
    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(const auto& auth : key_seeds) {
        trx.sign(my_tester->get_private_key(auth), my_tester->control->get_chain_id());
    }
    trx.sign(fc::crypto::private_key(std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")), my_tester->control->get_chain_id());
    my_tester->push_transaction(trx);

    tokendb.read_asset(issfg.address, symbol(5, 1), ast);
    CHECK(1500000 == ast.amount());

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_transferft_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "from": "EVT6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
      "to": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "number" : "12.00000 S#0",
      "memo": "memo"
    }
    )=====";

    auto var    = fc::json::from_string(test_data);
    auto trft   = var.as<transferft>();
    trft.number = asset::from_string(string("150.00000 S#") + std::to_string(get_sym_id()));
    trft.from   = key;
    trft.to     = address(tester::get_public_key(N(to)));
    to_variant(trft, var);

    //transfer rft more than balance exception
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), balance_exception);

    trft.to.set_reserved();
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    trft.to.set_generated(".abc", "test", 123);
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    trft.to     = address(tester::get_public_key(N(to)));
    trft.number = asset::from_string(string("15.00000 S#") + std::to_string(get_sym_id()));
    to_variant(trft, var);
    key_seeds.push_back(N(to));
    my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    auto payer2 = address(N(fungible), name128::from_number(get_sym_id()), 0);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2), payer_exception);

    payer2 = address(N(.fungible), name128::from_number(get_sym_id()), 0);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2), charge_exceeded_exception);

    my_tester->add_money(payer2, asset(100'000'000, evt_sym()));
    my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2);

    auto& tokendb = my_tester->control->token_db();
    asset ast;
    tokendb.read_asset(address(tester::get_public_key(N(to))), symbol(5, get_sym_id()), ast);
    CHECK(3'000'000 == ast.amount());

    //from == to test
    trft.from = address(tester::get_public_key(N(to)));
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), fungible_address_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_recycleft_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "address": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "number": "5.00000 S#1",
        "memo": "memo"
    }
    )=======";

    auto var   = fc::json::from_string(test_data);
    auto rf    = var.as<recycleft>();
    rf.number  = asset::from_string(string("1.00000 S#") + std::to_string(get_sym_id()));
    rf.address = address(tester::get_public_key(N(to)));
    to_variant(rf, var);

    auto& tokendb = my_tester->control->token_db();

    CHECK_THROWS_AS(my_tester->push_action(N(recycleft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    rf.address = poorer;
    to_variant(rf, var);

    CHECK_THROWS_AS(my_tester->push_action(N(recycleft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), balance_exception);

    rf.address = key;
    to_variant(rf, var);

    address fungible_address = address(N(.fungible), (fungible_name)std::to_string(get_sym_id()), 0);
    asset ast_from_before;
    asset ast_to_before;
    tokendb.read_asset(rf.address, symbol(5, get_sym_id()), ast_from_before);
    tokendb.read_asset_no_throw(fungible_address, symbol(5, get_sym_id()), ast_to_before);

    my_tester->push_action(N(recycleft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    asset ast_from_after;
    asset ast_to_after;
    tokendb.read_asset(rf.address, symbol(5, get_sym_id()), ast_from_after);
    tokendb.read_asset(fungible_address, symbol(5, get_sym_id()), ast_to_after);
    CHECK(100000 == ast_from_before.amount() - ast_from_after.amount());
    CHECK(100000 == ast_to_after.amount() - ast_to_before.amount());
}

TEST_CASE_METHOD(contracts_test, "contract_destroyft_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "address": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "number": "5.00000 S#1",
        "memo": "memo"
    }
    )=======";

    auto var   = fc::json::from_string(test_data);
    auto rf    = var.as<destroyft>();
    rf.number  = asset::from_string(string("1.00000 S#") + std::to_string(get_sym_id()));
    rf.address = address(tester::get_public_key(N(to)));
    to_variant(rf, var);

    auto& tokendb = my_tester->control->token_db();

    CHECK_THROWS_AS(my_tester->push_action(N(destroyft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    rf.address = poorer;
    to_variant(rf, var);

    CHECK_THROWS_AS(my_tester->push_action(N(destroyft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), balance_exception);

    rf.address = key;
    to_variant(rf, var);

    asset ast_from_before;
    asset ast_to_before;
    tokendb.read_asset(rf.address, symbol(5, get_sym_id()), ast_from_before);
    tokendb.read_asset_no_throw(address(), symbol(5, get_sym_id()), ast_to_before);

    my_tester->push_action(N(destroyft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    asset ast_from_after;
    asset ast_to_after;
    tokendb.read_asset(rf.address, symbol(5, get_sym_id()), ast_from_after);
    tokendb.read_asset(address(), symbol(5, get_sym_id()), ast_to_after);
    CHECK(100000 == ast_from_before.amount() - ast_from_after.amount());
    CHECK(100000 == ast_to_after.amount() - ast_to_before.amount());
}

TEST_CASE_METHOD(contracts_test, "contract_updatedomain_test", "[contracts]") {
    CHECK(true);
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
                "ref": "[G] .OWNER",
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

    auto  var     = fc::json::from_string(test_data);
    auto  updom   = var.as<updatedomain>();
    auto& tokendb = my_tester->control->token_db();

    domain_def dom;
    tokendb.read_domain(get_domain_name(), dom);
    CHECK(1 == dom.issue.authorizers[0].weight);

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(updatedomain), name128(get_domain_name()), N128(.update), var.get_object(), key_seeds, payer), action_authorize_exception);

    updom.name = get_domain_name();
    updom.issue->authorizers[0].ref.set_group(get_group_name());
    updom.manage->authorizers[0].ref.set_account(key);
    to_variant(updom, var);

    my_tester->push_action(N(updatedomain), name128(get_domain_name()), N128(.update), var.get_object(), key_seeds, payer);

    tokendb.read_domain(get_domain_name(), dom);
    CHECK(2 == dom.issue.authorizers[0].weight);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_group_auth_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
        "domain": "domain",
        "names": [
          "authorizers1"
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

    std::vector<account_name> seeds1 = {N(key0), N(key1), N(key2), N(key3), N(payer)};
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds1, payer), unsatisfied_authorization);

    istk.names[0] = "authorizers2";
    to_variant(istk, var);
    std::vector<account_name> seeds2 = {N(key1), N(key2), N(key3), N(key4), N(payer)};
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds2, payer), unsatisfied_authorization);

    istk.names[0] = "authorizers3";
    to_variant(istk, var);
    std::vector<account_name> seeds3 = {N(key0), N(key1), N(key2), N(key3), N(key4), N(payer)};
    my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds3, payer);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_failsuspend_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "testsuspend",
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

    auto var   = fc::json::from_string(test_data);
    auto ndact = var.as<newsuspend>();
    ndact.name = get_suspend_name();

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
                "ref": "[G] .OWNER",
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

    auto newdomain_var = fc::json::from_string(newdomain_test_data);
    auto newdom        = newdomain_var.as<newdomain>();
    newdom.creator     = tester::get_public_key(N(suspend_key));
    to_variant(newdom, newdomain_var);
    ndact.trx.actions.push_back(my_tester->get_action(N(newdomain), N128(domain), N128(.create), newdomain_var.get_object()));

    to_variant(ndact, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newsuspend), N128(.suspend), name128(get_suspend_name()), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    ndact.proposer = key;
    to_variant(ndact, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newsuspend), N128(.suspend), name128(get_suspend_name()), var.get_object(), key_seeds, payer), invalid_ref_block_exception);

    ndact.trx.set_reference_block(my_tester->control->head_block_id());
    to_variant(ndact, var);
    my_tester->push_action(N(newsuspend), N128(.suspend), name128(get_suspend_name()), var.get_object(), key_seeds, payer);

    const char* execute_test_data = R"=======(
    {
        "name": "testsuspend",
        "executor": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3"
    }
    )=======";

    auto execute_tvar = fc::json::from_string(execute_test_data);
    auto edact        = execute_tvar.as<execsuspend>();
    edact.executor    = key;
    edact.name        = get_suspend_name();
    to_variant(edact, execute_tvar);

    //suspend_executor_exception
    CHECK_THROWS_AS(my_tester->push_action(N(execsuspend), N128(.suspend), name128(get_suspend_name()), execute_tvar.get_object(), {N(key), N(payer)}, payer), suspend_executor_exception);

    auto&       tokendb = my_tester->control->token_db();
    suspend_def suspend;
    tokendb.read_suspend(edact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    auto        sig               = tester::get_private_key(N(suspend_key)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));
    auto        sig2              = tester::get_private_key(N(key)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));
    const char* approve_test_data = R"=======(
    {
        "name": "testsuspend",
        "signatures": [
        ]
    }
    )=======";

    auto approve_var = fc::json::from_string(approve_test_data);
    auto adact       = approve_var.as<aprvsuspend>();
    adact.name       = get_suspend_name();
    adact.signatures = {sig, sig2};
    to_variant(adact, approve_var);

    CHECK_THROWS_AS(my_tester->push_action(N(aprvsuspend), N128(.suspend), name128(get_suspend_name()), approve_var.get_object(), key_seeds, payer), suspend_not_required_keys_exception);

    tokendb.read_suspend(edact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    const char* cancel_test_data = R"=======(
    {
        "name": "testsuspend"
    }
    )=======";
    auto        cancel_var       = fc::json::from_string(test_data);
    auto        cdact            = var.as<cancelsuspend>();
    cdact.name                   = get_suspend_name();
    to_variant(cdact, cancel_var);

    my_tester->push_action(N(cancelsuspend), N128(.suspend), name128(get_suspend_name()), cancel_var.get_object(), key_seeds, payer);

    tokendb.read_suspend(edact.name, suspend);
    CHECK(suspend.status == suspend_status::cancelled);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_successsuspend_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "testsuspend",
        "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
        "trx": {
            "expiration": "2021-07-04T05:14:12",
            "ref_block_num": "3432",
            "ref_block_prefix": "291678901",
            "max_charge": 1000000,
            "actions": [
            ],
            "transaction_extensions": []
        }
    }
    )=======";

    auto var = fc::json::from_string(test_data);

    auto ndact      = var.as<newsuspend>();
    ndact.trx.payer = tester::get_public_key(N(payer));

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
                "ref": "[G] .OWNER",
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

    auto newdomain_var = fc::json::from_string(newdomain_test_data);
    auto newdom        = newdomain_var.as<newdomain>();
    newdom.creator     = tester::get_public_key(N(suspend_key));
    to_variant(newdom, newdomain_var);

    ndact.trx.set_reference_block(my_tester->control->fork_db_head_block_id());
    ndact.trx.actions.push_back(my_tester->get_action(N(newdomain), N128(domain), N128(.create), newdomain_var.get_object()));

    to_variant(ndact, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newsuspend), N128(.suspend), N128(testsuspend), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    ndact.proposer = key;
    to_variant(ndact, var);

    my_tester->push_action(N(newsuspend), N128(.suspend), N128(testsuspend), var.get_object(), key_seeds, payer);

    auto&       tokendb = my_tester->control->token_db();
    suspend_def suspend;
    tokendb.read_suspend(ndact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    auto        sig               = tester::get_private_key(N(suspend_key)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));
    auto        sig_payer         = tester::get_private_key(N(payer)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));
    const char* approve_test_data = R"=======(
    {
        "name": "testsuspend",
        "signatures": [
        ]
    }
    )=======";

    auto approve_var = fc::json::from_string(approve_test_data);
    auto adact       = approve_var.as<aprvsuspend>();
    adact.signatures = {sig, sig_payer};
    to_variant(adact, approve_var);

    my_tester->push_action(N(aprvsuspend), N128(.suspend), N128(testsuspend), approve_var.get_object(), {N(payer)}, payer);

    tokendb.read_suspend(adact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    bool is_payer_signed = suspend.signed_keys.find(payer.get_public_key()) != suspend.signed_keys.end();
    CHECK(is_payer_signed == true);

    const char* execute_test_data = R"=======(
    {
        "name": "testsuspend",
        "executor": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3"
    }
    )=======";

    auto execute_tvar = fc::json::from_string(execute_test_data);
    auto edact        = execute_tvar.as<execsuspend>();
    edact.executor    = tester::get_public_key(N(suspend_key));
    to_variant(edact, execute_tvar);

    my_tester->push_action(N(execsuspend), N128(.suspend), N128(testsuspend), execute_tvar.get_object(), {N(suspend_key), N(payer)}, payer);

    tokendb.read_suspend(edact.name, suspend);
    CHECK(suspend.status == suspend_status::executed);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_charge_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=====(
    {
      "address": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "number" : "12.00000 S#3",
      "memo": "memo"
    }
    )=====";

    my_tester->produce_blocks();

    auto var   = fc::json::from_string(test_data);
    auto issfg = var.as<issuefungible>();

    auto& tokendb = my_tester->control->token_db();
    auto  pbs  = my_tester->control->pending_block_state();
    auto& prod = pbs->get_scheduled_producer(pbs->header.timestamp).block_signing_key;

    asset prodasset_before;
    asset prodasset_after;
    tokendb.read_asset_no_throw(prod, evt_sym(), prodasset_before);

    issfg.number  = asset::from_string(string("5.00000 S#") + std::to_string(get_sym_id()));
    issfg.address = key;
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, poorer), charge_exceeded_exception);

    std::vector<account_name> tmp_seeds = {N(key), N(payer)};
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), tmp_seeds, poorer), payer_exception);

    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, address()), payer_exception);

    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, address(N(.notdomain), "domain", 0)), payer_exception);

    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, address(N(.domain), "domain", 0)), payer_exception);

    auto trace = my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    my_tester->produce_blocks();

    tokendb.read_asset_no_throw(prod, evt_sym(), prodasset_after);

    CHECK(trace->charge == prodasset_after.amount() - prodasset_before.amount());
}

TEST_CASE_METHOD(contracts_test, "contract_evt2pevt_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "from": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
        "to": "EVT548LviBDF6EcknKnKUMeaPUrZN2uhfCB1XrwHsURZngakYq9Vx",
        "number": "5.00000 S#4",
        "memo": "memo"
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  e2p     = var.as<evt2pevt>();
    auto& tokendb = my_tester->control->token_db();

    e2p.from = payer;
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(evt2pevt), N128(.fungible), (name128)std::to_string(evt_sym().id()), var.get_object(), key_seeds, payer), fungible_symbol_exception);

    e2p.number = asset::from_string(string("5.00000 S#1"));
    e2p.to.set_reserved();
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(evt2pevt), N128(.fungible), (name128)std::to_string(evt_sym().id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    e2p.to.set_generated(".hi", "test", 123);
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(evt2pevt), N128(.fungible), (name128)std::to_string(evt_sym().id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    e2p.to = key;
    to_variant(e2p, var);
    my_tester->push_action(N(evt2pevt), N128(.fungible), (name128)std::to_string(evt_sym().id()), var.get_object(), key_seeds, payer);

    asset ast;
    tokendb.read_asset(key, pevt_sym(), ast);
    CHECK(500000 == ast.amount());

    auto tf = var.as<transferft>();
    tf.from = key;
    tf.to   = payer;
    tf.number = asset(50, symbol(5,2));

    to_variant(tf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(pevt_sym().id()), var.get_object(), key_seeds, payer), fungible_symbol_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "everipass_test", "[contracts]") {
    auto link   = evt_link();
    auto header = 0;
    header |= evt_link::version1;
    header |= evt_link::everiPass;

    auto head_ts = my_tester->control->head_block_time().sec_since_epoch();

    link.set_header(header);
    link.add_segment(evt_link::segment(evt_link::timestamp, head_ts));
    link.add_segment(evt_link::segment(evt_link::domain, get_domain_name()));
    link.add_segment(evt_link::segment(evt_link::token, "t3"));

    auto ep = everipass();
    ep.link = link;

    auto sign_link = [&](auto& l) {
        l.clear_signatures();
        l.sign(private_key);
    };

    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t2), ep), key_seeds, payer), action_authorize_exception);

    ep.link.set_header(0);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), evt_link_version_exception);

    ep.link.set_header(evt_link::version1);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), evt_link_type_exception);

    ep.link.set_header(evt_link::version1 | evt_link::everiPay);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), evt_link_type_exception);

    ep.link.set_header(header);
    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts - 40));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), evt_link_expiration_exception);

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts + 40));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), evt_link_expiration_exception);

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts - 5));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts + 5));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer));

    // because t1 has two owners, here we only provide one
    ep.link.add_segment(evt_link::segment(evt_link::token, "t1"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t1), ep), key_seeds, payer), everipass_exception);

    ep.link.add_segment(evt_link::segment(evt_link::token, "t3"));
    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::token, "t5"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t5), ep), key_seeds, payer), unknown_token_exception);

    header |= evt_link::destroy;
    ep.link.set_header(header);
    ep.link.add_segment(evt_link::segment(evt_link::token, "t3"));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts - 1));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(get_domain_name(), N128(t3), ep), key_seeds, payer), token_destroyed_exception);
}

TEST_CASE_METHOD(contracts_test, "everipay_test", "[contracts]") {
    auto link   = evt_link();
    auto header = 0;
    header |= evt_link::version1;
    header |= evt_link::everiPay;

    auto head_ts = my_tester->control->head_block_time().sec_since_epoch();

    link.set_header(header);
    link.add_segment(evt_link::segment(evt_link::timestamp, head_ts));
    link.add_segment(evt_link::segment(evt_link::max_pay_str, "50000000"));
    link.add_segment(evt_link::segment(evt_link::symbol_id, evt_sym().id()));
    link.add_segment(evt_link::segment(evt_link::link_id, "KIJHNHFMJDUKJUAA"));

    auto ep   = everipay();
    ep.link   = link;
    ep.payee  = poorer;
    ep.number = asset::from_string(string("0.50000 S#1"));

    auto sign_link = [&](auto& l) {
        l.clear_signatures();
        l.sign(tester::get_private_key(N(payer)));
    };

    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(2), ep), key_seeds, payer), action_authorize_exception);

    ep.link.set_header(0);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), evt_link_version_exception);

    ep.link.set_header(evt_link::version1);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), evt_link_type_exception);

    ep.link.set_header(evt_link::version1 | evt_link::everiPass);
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer), evt_link_type_exception);

    ep.link.set_header(evt_link::version1 | evt_link::everiPay);
    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts - 40));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), evt_link_expiration_exception);

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts + 40));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), evt_link_expiration_exception);

    CHECK_THROWS_AS(my_tester->control->get_link_obj_for_link_id(ep.link.get_link_id()), evt_link_existed_exception);

    ep.link.add_segment(evt_link::segment(evt_link::link_id, "JKHBJKBJKGJHGJAA"));
    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts + 5));
    sign_link(ep.link);
    ep.payee.set_generated(".hi", "test", 123);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), address_reserved_exception);

    ep.payee.set_reserved();
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), (name128)std::to_string(evt_sym().id()), ep), key_seeds, payer), address_reserved_exception);

    ep.payee = poorer;
    CHECK_NOTHROW(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::link_id, "KIJHNHFMJDFFUKJU"));
    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts - 5));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::timestamp, head_ts));
    ep.link.add_segment(evt_link::segment(evt_link::link_id, "KIJHNHFMJDFFUKJU"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer), evt_link_dupe_exception);

    ep.link.add_segment(evt_link::segment(evt_link::link_id, "JKHBJKBJKGJHGJKG"));
    ep.number = asset::from_string(string("5.00000 S#1"));
    sign_link(ep.link);
    CHECK_NOTHROW(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer));

    ep.link.add_segment(evt_link::segment(evt_link::max_pay_str, "5000"));
    ep.link.add_segment(evt_link::segment(evt_link::link_id, "JKHBJKBJKGJHGJKB"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer), everipay_exception);

    ep.payee = payer;
    ep.link.add_segment(evt_link::segment(evt_link::link_id, "JKHBJKBJKGJHGJKA"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer), everipay_exception);

    ep.number = asset::from_string(string("500.00000 S#2"));
    ep.link.add_segment(evt_link::segment(evt_link::link_id, "JKHBJKBJKGJHGJKE"));
    sign_link(ep.link);
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.fungible), N128(1), ep), key_seeds, payer), everipay_exception);
}

TEST_CASE_METHOD(contracts_test, "empty_action_test", "[contracts]") {
    CHECK(true);
    auto trx = signed_transaction();
    my_tester->set_transaction_headers(trx, payer);

    CHECK_THROWS_AS(my_tester->push_transaction(trx), tx_no_action);
}

TEST_CASE_METHOD(contracts_test, "contract_addmeta_test", "[contracts]") {
    CHECK(true);
    my_tester->add_money(payer, asset(10'000'000, symbol(5, EVT_SYM_ID)));

    const char* test_data = R"=====(
    {
      "key": "key",
      "value": "value'f\"\n\t",
      "creator": "[A] EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
    }
    )=====";

    auto var  = fc::json::from_string(test_data);
    auto admt = var.as<addmeta>();

    //meta authorizers test
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    //meta authorizers test
    admt.creator = tester::get_public_key(N(other));
    to_variant(admt, var);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(.meta), var.get_object(), {N(other), N(payer)}, payer, 5'000'000), meta_involve_exception);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.group), name128(get_group_name()), var.get_object(), {N(other), N(payer)}, payer, 5'000'000), meta_involve_exception);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), {N(other), N(payer)}, payer, 5'000'000), meta_involve_exception);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(t1), var.get_object(), {N(other), N(payer)}, payer, 5'000'000), meta_involve_exception);

    admt.creator = key;
    to_variant(admt, var);

    my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds, payer, 5'000'000);
    my_tester->push_action(N(addmeta), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer, 5'000'000);
    my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer, 5'000'000);
    my_tester->push_action(N(addmeta), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer, 5'000'000);

    //meta_key_exception test

    admt.creator = key;
    admt.value   = "value2";
    to_variant(admt, var);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(.meta), var.get_object(), key_seeds, payer, 5'000'000), meta_key_exception);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.group), name128(get_group_name()), var.get_object(), key_seeds, payer, 5'000'000), meta_key_exception);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer, 5'000'000), meta_key_exception);

    admt.creator = tester::get_public_key(N(key2));
    to_variant(admt, var);
    CHECK_THROWS_AS(my_tester->push_action(N(addmeta), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), {N(key2), N(payer)}, payer, 5'000'000), meta_key_exception);

    std::vector<account_name> seeds = {N(key0), N(key1), N(key2), N(key3), N(key4), N(payer)};

    const char* domain_data = R"=====(
        {
          "name" : "gdomain",
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
                "ref": "[G] .OWNER",
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

    auto domain_var = fc::json::from_string(domain_data);
    auto newdom     = domain_var.as<newdomain>();

    newdom.creator = key;
    newdom.issue.authorizers[0].ref.set_group(get_group_name());
    newdom.manage.authorizers[0].ref.set_group(get_group_name());
    to_variant(newdom, domain_var);

    my_tester->push_action(N(newdomain), N128(gdomain), N128(.create), domain_var.get_object(), key_seeds, payer);

    const char* tk_data = R"=====(
    {
      "domain": "gdomain",
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

    auto tk_var = fc::json::from_string(tk_data);
    auto istk   = tk_var.as<issuetoken>();

    my_tester->push_action(N(issuetoken), N128(gdomain), N128(.issue), tk_var.get_object(), seeds, payer);

    const char* fg_data = R"=====(
    {
      "name": "GEVT",
      "sym_name": "GEVT",
      "sym": "5,S#4",
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
      "total_supply":"100.00000 S#4"
    }
    )=====";

    auto fg_var = fc::json::from_string(fg_data);
    auto newfg  = fg_var.as<newfungible>();

    newfg.creator = key;
    newfg.issue.authorizers[0].ref.set_account(key);
    newfg.manage.authorizers[0].ref.set_group(get_group_name());
    to_variant(newfg, fg_var);
    my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id() + 1), fg_var.get_object(), key_seeds, payer);

    admt.creator.set_group(get_group_name());
    admt.key = "key2";
    to_variant(admt, var);

    my_tester->push_action(N(addmeta), N128(gdomain), N128(.meta), var.get_object(), seeds, payer, 5'000'000);
    my_tester->push_action(N(addmeta), N128(.fungible), (name128)std::to_string(get_sym_id() + 1), var.get_object(), seeds, payer, 5'000'000);
    my_tester->push_action(N(addmeta), N128(gdomain), N128(t1), var.get_object(), seeds, payer, 5'000'000);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_prodvote_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "producer": "evt",
        "key": "key",
        "value": 123456789
    }
    )=======";

    auto var    = fc::json::from_string(test_data);
    auto pv   = var.as<prodvote>();
    auto& tokendb = my_tester->control->token_db();

    std::map<public_key_type, int> vote_sum;
    tokendb.read_prodvotes_no_throw(pv.key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 0);

    pv.key = N128(network-charge-factor);
    to_variant(pv, var);

    CHECK_THROWS_AS(my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-charge-factor), var.get_object(), {N(payer)}, payer), unsatisfied_authorization);

    pv.value = 1'000'000;
    to_variant(pv, var);
    CHECK_THROWS_AS(my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-charge-factor), var.get_object(), key_seeds, payer), prodvote_value_exception);

    pv.value = 0;
    to_variant(pv, var);
    CHECK_THROWS_AS(my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-charge-factor), var.get_object(), key_seeds, payer), prodvote_value_exception);

    pv.value = 1;
    to_variant(pv, var);
    my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-charge-factor), var.get_object(), key_seeds, payer);

    tokendb.read_prodvotes_no_throw(pv.key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 1);
    CHECK(my_tester->control->get_global_properties().configuration.base_network_charge_factor == 1);

    pv.value = 10;
    to_variant(pv, var);
    my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-charge-factor), var.get_object(), key_seeds, payer);
    CHECK(my_tester->control->get_global_properties().configuration.base_network_charge_factor == 10);

    pv.key = N128(storage-charge-factor);
    to_variant(pv, var);
    my_tester->push_action(N(prodvote), N128(.prodvote), N128(storage-charge-factor), var.get_object(), key_seeds, payer);
    CHECK(my_tester->control->get_global_properties().configuration.base_storage_charge_factor == 10);

    pv.key = N128(cpu-charge-factor);
    to_variant(pv, var);
    my_tester->push_action(N(prodvote), N128(.prodvote), N128(cpu-charge-factor), var.get_object(), key_seeds, payer);
    CHECK(my_tester->control->get_global_properties().configuration.base_cpu_charge_factor == 10);

    pv.key = N128(global-charge-factor);
    to_variant(pv, var);
    my_tester->push_action(N(prodvote), N128(.prodvote), N128(global-charge-factor), var.get_object(), key_seeds, payer);
    CHECK(my_tester->control->get_global_properties().configuration.global_charge_factor == 10);

    pv.key = N128(network-fuck-factor);
    to_variant(pv, var);
    CHECK_THROWS_AS(my_tester->push_action(N(prodvote), N128(.prodvote), N128(network-fuck-factor), var.get_object(), key_seeds, payer), prodvote_key_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_updsched_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "producers": [{
            "producer_name": "producer",
            "block_signing_key": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        }]
    }
    )=======";

    auto var    = fc::json::from_string(test_data);
    auto us   = var.as<updsched>();
    auto& tokendb = my_tester->control->token_db();

    us.producers[0].block_signing_key = tester::get_public_key("evt");
    to_variant(us, var);

    signed_transaction trx;
    trx.actions.emplace_back(my_tester->get_action(N(updsched), N128(.prodsched), N128(.update),  var.get_object()));
    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(const auto& auth : key_seeds) {
        trx.sign(my_tester->get_private_key(auth), my_tester->control->get_chain_id());
    }
    trx.sign(fc::crypto::private_key(std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")), my_tester->control->get_chain_id());
    my_tester->push_transaction(trx);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_newnftlock_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "proposer": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "unlock_time": "2020-06-09T09:06:27",
        "deadline": "2020-07-09T09:06:27",
        "assets": [{
            "type": "tokens",
            "data": {
                "domain": "cookie",
                "names": [
                    "t3"
                ]
            }
        }],
        "condition": {
            "type": "cond_keys",
            "data": {
                "threshold": 1,
                "cond_keys": [
                    "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                    "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
                ]
            }
        },
        "succeed": [
        ],
        "failed": [
            "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        ]
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  nl      = var.as<newlock>();
    auto& tokendb = my_tester->control->token_db();
    CHECK(!tokendb.exists_lock(nl.name));

    auto now       = fc::time_point::now();
    nl.unlock_time = now + fc::days(10);
    nl.deadline    = now + fc::days(20);

    CHECK(nl.assets[0].type() == asset_type::tokens);
    nl.assets[0].get<locknft_def>().domain = get_domain_name();
    to_variant(nl, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    nl.proposer = tester::get_public_key(N(key));
    nl.condition.get<lock_condkeys>().cond_keys = {tester::get_public_key(N(key))};
    to_variant(nl, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_address_exception);

    nl.succeed = {public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))};
    to_variant(nl, var);

    my_tester->push_action(N(newlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    CHECK(tokendb.exists_lock(nl.name));

    lock_def lock_;
    tokendb.read_lock(nl.name, lock_);
    CHECK(lock_.status == lock_status::proposed);

    token_def tk;
    tokendb.read_token(get_domain_name(), "t3", tk);
    CHECK(tk.owner.size() == 1);
    CHECK(tk.owner[0] == address(N(.lock), N128(nlact.name), 0));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_newftlock_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "ftlock",
        "proposer": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "unlock_time": "2020-06-09T09:06:27",
        "deadline": "2020-07-09T09:06:27",
        "assets": [{
            "type": "fungible",
            "data": {
                "from": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                "amount": "5.00000 S#2"
            }
        }],
        "condition": {
            "type": "cond_keys",
            "data": {
                "threshold": 3,
                "cond_keys": [
                    "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                    "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
                ]
            }
        },
        "succeed": [
        ],
        "failed": [
            "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        ]
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  nl      = var.as<newlock>();
    auto& tokendb = my_tester->control->token_db();
    CHECK(!tokendb.exists_lock(nl.name));

    auto now       = fc::time_point::now();
    nl.unlock_time = now + fc::days(10);
    nl.deadline    = now + fc::days(20);

    nl.proposer = tester::get_public_key(N(key));
    nl.condition.get<lock_condkeys>().cond_keys = {tester::get_public_key(N(key))};
    to_variant(nl, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_condition_exception);

    auto& cond = nl.condition.get<lock_condkeys>();
    cond.threshold = 1;
    to_variant(nl, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_assets_exception);

    auto& ft = nl.assets[0].get<lockft_def>();
    ft.amount = asset::from_string(string("5.00000 S#") + std::to_string(get_sym_id()));
    ft.from   = tester::get_public_key(N(key));
    to_variant(nl, var);

    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_address_exception);

    nl.succeed = { tester::get_public_key(N(key)), tester::get_public_key(N(key2)) };
    to_variant(nl, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_address_exception);

    nl.succeed = {address()};
    to_variant(nl, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), address_reserved_exception);

    nl.succeed = {address(".123", "test", 123)};
    to_variant(nl, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), address_reserved_exception);

    nl.succeed = {public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))};
    to_variant(nl, var);
    my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000);

    CHECK(tokendb.exists_lock(nl.name));

    lock_def lock_;
    tokendb.read_lock(nl.name, lock_);
    CHECK(lock_.status == lock_status::proposed);

    asset ast;
    tokendb.read_asset(address(N(.lock), N128(nlact.name), 0), nl.assets[0].get<lockft_def>().amount.sym(), ast);
    CHECK(ast.amount() == 500000);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_aprvlock_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "approver": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "data": {
            "type": "cond_key"
        }
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  al      = var.as<aprvlock>();
    auto& tokendb = my_tester->control->token_db();

    lock_def lock_;
    tokendb.read_lock(al.name, lock_);
    CHECK(lock_.signed_keys.size() == 0);

    CHECK_THROWS_AS(my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    al.approver = {tester::get_public_key(N(payer))};
    to_variant(al, var);
    CHECK_THROWS_AS(my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_aprv_data_exception);

    al.approver = {tester::get_public_key(N(key))};
    to_variant(al, var);
    
    my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    tokendb.read_lock(al.name, lock_);
    CHECK(lock_.signed_keys.size() == 1);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_tryunlock_test", "[contracts]") {
    CHECK(true);
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "executor": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  tul     = var.as<tryunlock>();
    auto& tokendb = my_tester->control->token_db();

    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    tul.executor = {tester::get_public_key(N(key))};
    to_variant(tul, var);
    
    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_not_reach_unlock_time);

    my_tester->produce_block();
    my_tester->produce_block(fc::days(12));
    my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    lock_def lock_;
    tokendb.read_lock(tul.name, lock_);
    CHECK(lock_.status == lock_status::succeed);

    token_def tk;
    tokendb.read_token(get_domain_name(), "t3", tk);
    CHECK(tk.owner.size() == 1);
    CHECK(tk.owner[0] == public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC")));

    tul.name = N128(ftlock);
    to_variant(tul, var);

    // not reach deadline
    // not all conditional keys are signed
    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_not_reach_deadline);

    lock_def ft_lock;
    tokendb.read_lock(N128(ftlock), ft_lock);
    CHECK(ft_lock.status == lock_status::proposed);

    my_tester->produce_block();
    my_tester->produce_block(fc::days(12));

    // exceed deadline, turn into failed
    my_tester->push_action(N(tryunlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000);

    tokendb.read_lock(N128(ftlock), ft_lock);
    CHECK(ft_lock.status == lock_status::failed);

    // failed address
    asset ast;
    tokendb.read_asset(address(public_key_type(std::string("EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"))), symbol(5, get_sym_id()), ast);
    CHECK(ast.amount() == 500000);

    my_tester->produce_blocks();
}
