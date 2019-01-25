#include "contracts_tests.hpp"

TEST_CASE_METHOD(contracts_test, "contract_charge_test", "[contracts]") {
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
    READ_ASSET(prod, evt_sym(), prodasset_before);

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

    READ_ASSET(prod, evt_sym(), prodasset_after);

    CHECK(trace->charge == prodasset_after.amount() - prodasset_before.amount());
}

TEST_CASE_METHOD(contracts_test, "empty_action_test", "[contracts]") {
    auto trx = signed_transaction();
    my_tester->set_transaction_headers(trx, payer);

    CHECK_THROWS_AS(my_tester->push_transaction(trx), tx_no_action);
}

TEST_CASE_METHOD(contracts_test, "contract_addmeta_test", "[contracts]") {
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
    const char* test_data = R"=======(
    {
        "producer": "evt",
        "key": "key",
        "value": 123456789
    }
    )=======";

    auto var  = fc::json::from_string(test_data);
    auto pv   = var.as<prodvote>();
    auto& tokendb = my_tester->control->token_db();

    auto vote_sum = flat_map<public_key_type, int64_t>();

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

    READ_TOKEN(prodvote, pv.key, vote_sum);
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
