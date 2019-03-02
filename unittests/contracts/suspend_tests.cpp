#include "contracts_tests.hpp"

auto CHECK_EQUAL = [](auto& lhs, auto& rhs) {
    auto b1 = fc::raw::pack(lhs);
    auto b2 = fc::raw::pack(rhs);

    CHECK(b1.size() == b2.size());
    CHECK(memcmp(b1.data(), b2.data(), b1.size()) == 0);
};

TEST_CASE_METHOD(contracts_test, "contract_failsuspend_test", "[contracts]") {
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
    auto  cache = token_database_cache(tokendb, 1024 * 1024);
    suspend_def suspend;
    READ_TOKEN(suspend, edact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    auto sig  = tester::get_private_key(N(suspend_key)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));
    auto sig2 = tester::get_private_key(N(key)).sign(suspend.trx.sig_digest(my_tester->control->get_chain_id()));

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

    READ_TOKEN(suspend, edact.name, suspend);
    CHECK(suspend.status == suspend_status::proposed);

    const char* cancel_test_data = R"=======(
    {
        "name": "testsuspend"
    }
    )=======";

    auto cancel_var = fc::json::from_string(test_data);
    auto cdact      = var.as<cancelsuspend>();
    cdact.name      = get_suspend_name();
    to_variant(cdact, cancel_var);

    my_tester->push_action(N(cancelsuspend), N128(.suspend), name128(get_suspend_name()), cancel_var.get_object(), key_seeds, payer);

    READ_TOKEN(suspend, edact.name, suspend);
    CHECK(suspend.status == suspend_status::cancelled);

    my_tester->produce_blocks();

    auto suspend2 = cache.read_token<suspend_def>(token_type::token, N128(.suspend), name128(get_suspend_name()));
    CHECK(suspend2 != nullptr);
    CHECK_EQUAL(suspend, *suspend2);
}

TEST_CASE_METHOD(contracts_test, "contract_successsuspend_test", "[contracts]") {
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

    auto& tokendb = my_tester->control->token_db();
    auto  cache = token_database_cache(tokendb, 1024 * 1024);
    auto  suspend = suspend_def();
    READ_TOKEN(suspend, ndact.name, suspend);
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

    READ_TOKEN(suspend, adact.name, suspend);
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

    READ_TOKEN(suspend, edact.name, suspend);
    CHECK(suspend.status == suspend_status::executed);

    my_tester->produce_blocks();

    READ_TOKEN(suspend, adact.name, suspend);
    auto suspend2 = cache.read_token<suspend_def>(token_type::token, N128(.suspend), adact.name);
    CHECK(suspend2 != nullptr);
    CHECK_EQUAL(suspend, *suspend2);
}
