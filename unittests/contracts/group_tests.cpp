#include "contracts_tests.hpp"

auto CHECK_EQUAL = [](auto& lhs, auto& rhs) {
    auto b1 = fc::raw::pack(lhs);
    auto b2 = fc::raw::pack(rhs);

    CHECK(b1.size() == b2.size());
    CHECK(memcmp(b1.data(), b2.data(), b1.size()) == 0);
};

TEST_CASE_METHOD(contracts_test, "newgroup_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "name" : "5jxX",
      "group" : {
        "name": "5jxXg",
        "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "root": {
          "threshold": 6,
          "weight": 0,
          "nodes": [{
              "threshold": 2,
              "weight": 6,
              "nodes": [{
                  "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                  "weight": 1
                },{
                  "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 1
                }
              ]
            },{
              "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
              "weight": 3
            },{
              "threshold": 2,
              "weight": 3,
              "nodes": [{
                  "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                  "weight": 1
                },{
                  "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
    auto& cache       = my_tester->control->token_db_cache();

    CHECK(!EXISTS_TOKEN(group, get_group_name()));
    my_tester->add_money(group_payer, asset(10'000'000, symbol(5, jmzk_SYM_ID)));

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

    CHECK(EXISTS_TOKEN(group, get_group_name()));

    my_tester->produce_blocks();

    auto group = group_def();
    READ_TOKEN2(token, N128(.group), name128(get_group_name()), group);
    auto group2 = cache.read_token<group_def>(token_type::token, N128(.group), name128(get_group_name()));
    CHECK(group2 != nullptr);
    CHECK_EQUAL(group, *group2);
}

TEST_CASE_METHOD(contracts_test, "updategroup_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "name" : "5jxX",
      "group" : {
        "name": "5jxXg",
        "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "root": {
          "threshold": 5,
          "weight": 0,
          "nodes": [{
              "threshold": 2,
              "weight": 2,
              "nodes": [{
                  "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                  "weight": 1
                },{
                  "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 1
                }
              ]
            },{
              "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
              "weight": 1
            },{
              "threshold": 2,
              "weight": 2,
              "nodes": [{
                  "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                  "weight": 1
                },{
                  "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
    auto& cache   = my_tester->control->token_db_cache();

    CHECK(EXISTS_TOKEN(group, get_group_name()));
    group gp;
    READ_TOKEN(group, get_group_name(), gp);
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

    READ_TOKEN(group, get_group_name(), gp);
    CHECK(5 == gp.root().threshold);

    my_tester->produce_blocks();

    auto group = group_def();
    READ_TOKEN2(token, N128(.group), name128(get_group_name()), group);
    auto group2 = cache.read_token<group_def>(token_type::token, N128(.group), name128(get_group_name()));
    CHECK(group2 != nullptr);
    CHECK_EQUAL(group, *group2);
}

TEST_CASE_METHOD(contracts_test, "updatedomain_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "name" : "domain",
      "issue" : {
        "name": "issue",
        "threshold": 2,
        "authorizers": [{
          "ref": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
            "ref": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
    READ_TOKEN(domain, get_domain_name(), dom);
    CHECK(1 == dom.issue.authorizers[0].weight);

    //action_authorize_exception test
    CHECK_THROWS_AS(my_tester->push_action(N(updatedomain), name128(get_domain_name()), N128(.update), var.get_object(), key_seeds, payer), action_authorize_exception);

    updom.name = get_domain_name();
    updom.issue->authorizers[0].ref.set_group(get_group_name());
    updom.transfer->authorizers[0].ref.set_account(key);
    updom.manage->authorizers[0].ref.set_account(key);
    to_variant(updom, var);

    my_tester->push_action(N(updatedomain), name128(get_domain_name()), N128(.update), var.get_object(), key_seeds, payer);

    READ_TOKEN(domain, get_domain_name(), dom);
    CHECK(2 == dom.issue.authorizers[0].weight);

    // add `.disable-set-transfer` with 'true' to domain-1
    auto am    = addmeta();
    am.key     = N128(.disable-set-transfer);
    am.value   = "true";
    am.creator = key; 

    my_tester->push_action(action(get_domain_name(1), N128(.meta), am), key_seeds, payer, 5'000'000);

    updom.name = get_domain_name(1);
    to_variant(updom, var);
    CHECK_THROWS_AS(my_tester->push_action(N(updatedomain), name128(get_domain_name(1)), N128(.update), var.get_object(), key_seeds, payer), domain_cannot_update_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "group_auth_test", "[contracts]") {
    const char* test_data = R"=====(
    {
        "domain": "domain",
        "names": [
          "authorizers1"
        ],
        "owner": [
          "jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2"
        ]
    }
    )=====";

    auto var  = fc::json::from_string(test_data);
    auto istk = var.as<issuetoken>();

    istk.domain   = get_domain_name();
    istk.owner[0] = key;
    to_variant(istk, var);

    std::vector<name> seeds1 = {N(key0), N(key1), N(key2), N(key3), N(payer)};
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds1, payer), unsatisfied_authorization);

    istk.names[0] = "authorizers2";
    to_variant(istk, var);
    std::vector<name> seeds2 = {N(key1), N(key2), N(key3), N(key4), N(payer)};
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds2, payer), unsatisfied_authorization);

    istk.names[0] = "authorizers3";
    to_variant(istk, var);
    std::vector<name> seeds3 = {N(key0), N(key1), N(key2), N(key3), N(key4), N(payer)};
    my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), seeds3, payer);

    my_tester->produce_blocks();
}
