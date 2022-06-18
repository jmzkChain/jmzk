#include "contracts_tests.hpp"

TEST_CASE_METHOD(contracts_test, "multi_actions_test", "[contracts]") {
    signed_transaction trx;
    auto&              tokendb = my_tester->control->token_db();
    auto&              cache   = my_tester->control->token_db_cache();

    const char* test_data = R"=====(
        {
          "name" : "domain",
          "creator" : "jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
          "issue" : {
            "name" : "issue",
            "threshold" : 1,
            "authorizers": [{
                "ref": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
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
                "ref": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
                "weight": 1
              }
            ]
          }
        }
        )=====";

    auto var    = fc::json::from_string(test_data);
    auto newdom = var.as<newdomain>();

    auto domain_name = get_domain_name(2);
    newdom.creator   = key;
    newdom.name      = domain_name;
    newdom.issue.authorizers[0].ref.set_account(key);
    newdom.manage.authorizers[0].ref.set_account(key);
    to_variant(newdom, var);
    trx.actions.emplace_back(my_tester->get_action(N(newdomain), name128(domain_name), N128(.create), var.get_object()));

    auto am    = addmeta();
    am.key     = N128(key);
    am.value   = "value";
    am.creator = key;

    trx.actions.emplace_back(action(domain_name, N128(.meta), am));

    test_data = R"=====(
    {
      "domain": "domain",
        "names": [
          "t1",
          "t2",
          "t3",
          "t4",
          "t5"
        ],
        "owner": [
          "jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2"
        ]
    }
    )=====";

    var           = fc::json::from_string(test_data);
    auto istk     = var.as<issuetoken>();
    istk.domain   = newdom.name;
    istk.owner[0] = key;
    to_variant(istk, var);
    trx.actions.emplace_back(my_tester->get_action(N(issuetoken), name128(domain_name), N128(.issue), var.get_object()));

    test_data = R"=====(
    {
      "domain": "cookie",
      "name": "t1",
      "to": [
        "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
      ],
      "memo":"memo"
    }
    )=====";

    var        = fc::json::from_string(test_data);
    auto trf   = var.as<transfer>();
    trf.domain = domain_name;
    to_variant(trf, var);
    trx.actions.emplace_back(my_tester->get_action(N(transfer), name128(domain_name), N128(t1), var.get_object()));

    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(const auto& auth : key_seeds) {
        trx.sign(my_tester->get_private_key(auth), my_tester->control->get_chain_id());
    }

    my_tester->push_transaction(trx);

    token_def tk;
    READ_TOKEN2(token, domain_name, "t1", tk);
    CHECK(2 == tk.owner.size());

    my_tester->produce_blocks();
}