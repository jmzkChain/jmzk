#include "contracts_tests.hpp"

TEST_CASE_METHOD(contracts_test, "contract_newdomain_test", "[contracts]") {
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

    CHECK(!EXISTS_TOKEN(domain, get_domain_name()));

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

    CHECK(EXISTS_TOKEN(domain, get_domain_name()));

    newdom.name = get_domain_name(1);
    my_tester->push_action(action(newdom.name, N128(.create), newdom), key_seeds, payer);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_issuetoken_test", "[contracts]") {
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

    CHECK(!EXISTS_TOKEN2(token, get_domain_name(), "t1"));

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

    std::vector<name> v2 { "other", "payer" };
    CHECK_THROWS_AS(my_tester->push_action(N(issuetoken), name128(get_domain_name()), N128(.issue), var.get_object(), v2, payer), unsatisfied_authorization);

    CHECK(EXISTS_TOKEN2(token, get_domain_name(), "t1"));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_transfer_test", "[contracts]") {
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
    READ_TOKEN2(token, get_domain_name(), "t1", tk);
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

    READ_TOKEN2(token, get_domain_name(), "t1", tk);
    CHECK(2 == tk.owner.size());

    trf.to[1] = key;
    to_variant(trf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transfer), name128(get_domain_name()), N128(t1), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "contract_destroytoken_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "domain": "cookie",
      "name": "t2"
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  destk   = var.as<destroytoken>();
    auto& tokendb = my_tester->control->token_db();

    CHECK(EXISTS_TOKEN2(token, get_domain_name(), "t2"));

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
    READ_TOKEN2(token, get_domain_name(), "t2", tk);
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
