#include "contracts_tests.hpp"

auto CHECK_EQUAL = [](auto& lhs, auto& rhs) {
    auto b1 = fc::raw::pack(lhs);
    auto b2 = fc::raw::pack(rhs);

    CHECK(b1.size() == b2.size());
    CHECK(memcmp(b1.data(), b2.data(), b1.size()) == 0);
};

TEST_CASE_METHOD(contracts_test, "newfungible_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "name": "jmzk",
      "sym_name": "jmzk",
      "sym": "5,S#3",
      "creator": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
            "weight": 1
          }
        ]
      },
      "manage": {
        "name": "manage",
        "threshold": 1,
        "authorizers": [{
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
            "weight": 1
          }
        ]
      },
      "total_supply":"10000.00000 S#3"
    }
    )=====";

    auto var            = fc::json::from_string(test_data);
    auto fungible_payer = address(N(.domain), ".fungible", 0);
    my_tester->add_money(fungible_payer, asset(10'000'000, symbol(5, jmzk_SYM_ID)));
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();

    CHECK(!EXISTS_TOKEN(fungible, 3));

    auto newfg = var.as<newfungible>();

    newfg.name         = get_symbol_name();
    newfg.sym_name     = get_symbol_name();
    newfg.total_supply = asset::from_string(string("10000.00000 S#3"));
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
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer), fungible_duplicate_exception);

    newfg.total_supply = asset::from_string(string("0.00000 S#3"));
    to_variant(newfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, fungible_payer), fungible_supply_exception);

    CHECK(EXISTS_TOKEN(fungible, get_sym_id()));

    my_tester->produce_blocks();

    auto ft  = fungible_def();
    READ_TOKEN2(token, N128(.fungible), get_sym_id(), ft);
    auto ft2  = cache.read_token<fungible_def>(token_type::token, N128(.fungible), get_sym_id());
    CHECK(ft2 != nullptr);
    CHECK_EQUAL(ft, *ft2);
}

TEST_CASE_METHOD(contracts_test, "newfungible_v2_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "name": "jmzk",
      "sym_name": "jmzk",
      "sym": "5,S#5",
      "creator": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
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
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
            "weight": 1
          }
        ]
      },
      "total_supply":"10000.00000 S#5"
    }
    )=====";

    auto var            = fc::json::from_string(test_data);
    auto fungible_payer = address(N(.domain), ".fungible", 0);
    my_tester->add_money(fungible_payer, asset(10'000'000, symbol(get_sym_id(2), jmzk_SYM_ID)));
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();

    CHECK(!EXISTS_TOKEN(fungible, get_sym_id(2)));

    my_tester->control->get_execution_context().set_version(N(newfungible), 2);

    auto newfg = var.as<newfungible_v2>();

    newfg.name         = get_symbol_name();
    newfg.sym_name     = get_symbol_name();
    newfg.total_supply = asset::from_string(string("10000.00000 S#5"));
    to_variant(newfg, var);
    //new fungible authorization test
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id(2)), var.get_object(), key_seeds, fungible_payer), unsatisfied_authorization);

    newfg.creator = key;
    newfg.issue.authorizers[0].ref.set_account(key);
    newfg.manage.authorizers[0].ref.set_account(key);
    to_variant(newfg, var);
    my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id(2)), var.get_object(), key_seeds, fungible_payer);

    newfg.name         = "lala";
    newfg.sym_name     = "lala";
    newfg.total_supply = asset::from_string(string("10.00000 S#5"));
    to_variant(newfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id(2)), var.get_object(), key_seeds, fungible_payer), fungible_duplicate_exception);

    newfg.total_supply = asset::from_string(string("0.00000 S#5"));
    to_variant(newfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(newfungible), N128(.fungible), (name128)std::to_string(get_sym_id(2)), var.get_object(), key_seeds, fungible_payer), fungible_supply_exception);

    CHECK(EXISTS_TOKEN(fungible, get_sym_id(2)));

    my_tester->produce_blocks();

    auto ft  = fungible_def();
    READ_TOKEN2(token, N128(.fungible), get_sym_id(2), ft);
    auto ft2  = cache.read_token<fungible_def>(token_type::token, N128(.fungible), get_sym_id(2));
    CHECK(ft2 != nullptr);
    CHECK_EQUAL(ft, *ft2);

    // restore back
    my_tester->control->get_execution_context().set_version_unsafe(N(newfungible), 1);
    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "updfungible_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "sym_id": "0",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
            "weight": 2
          }
        ]
      },
      "manage": {
        "name": "manage",
        "threshold": 1,
        "authorizers": [{
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      }
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  updfg   = var.as<updfungible_v2>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache   = my_tester->control->token_db_cache();

    fungible_def fg;
    READ_TOKEN(fungible, get_sym_id(), fg);
    CHECK(1 == fg.issue.authorizers[0].weight);

    //action_authorize_exception test
    auto strkey = (std::string)key;
    CHECK_THROWS_AS(my_tester->push_action(N(updfungible), N128(.fungible), name128::from_number(get_sym_id()), var.get_object(), key_seeds, payer), action_authorize_exception);

    updfg.sym_id = get_sym_id();
    updfg.issue->authorizers[0].ref.set_account(key);
    updfg.manage->authorizers[0].ref.set_account(tester::get_public_key(N(key2)));
    to_variant(updfg, var);

    my_tester->push_action(N(updfungible), N128(.fungible), name128::from_number(get_sym_id()), var.get_object(), key_seeds, payer);

    READ_TOKEN(fungible, get_sym_id(), fg);
    CHECK(2 == fg.issue.authorizers[0].weight);

    my_tester->produce_blocks();

    auto ft  = fungible_def();
    READ_TOKEN2(token, N128(.fungible), get_sym_id(), ft);
    auto ft2  = cache.read_token<fungible_def>(token_type::token, N128(.fungible), get_sym_id());
    CHECK(ft2 != nullptr);
    CHECK_EQUAL(ft, *ft2);
}

TEST_CASE_METHOD(contracts_test, "updfungible_v2_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "sym_id": "0",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
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
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      }
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  updfg   = var.as<updfungible_v2>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache   = my_tester->control->token_db_cache();

    fungible_def fg;
    READ_TOKEN(fungible, get_sym_id(2), fg);
    CHECK(1 == fg.issue.authorizers[0].weight);

    my_tester->control->get_execution_context().set_version(N(updfungible), 2);

    //action_authorize_exception test
    auto strkey = (std::string)key;
    CHECK_THROWS_AS(my_tester->push_action(N(updfungible), N128(.fungible), name128::from_number(get_sym_id(2)), var.get_object(), key_seeds, payer), action_authorize_exception);

    updfg.sym_id = get_sym_id(2);
    updfg.issue->authorizers[0].ref.set_account(key);
    updfg.transfer->authorizers[0].ref.set_account(key);
    updfg.manage->authorizers[0].ref.set_account(tester::get_public_key(N(key2)));
    to_variant(updfg, var);

    // add `.disable-set-transfer` with 'true' to fungible 5
    auto am    = addmeta();
    am.key     = N128(.disable-set-transfer);
    am.value   = "true";
    am.creator = key; 

    my_tester->push_action(action(N128(.fungible), (name128)std::to_string(get_sym_id(2)), am), key_seeds, payer, 5'000'000);

    CHECK_THROWS_AS(my_tester->push_action(N(updfungible), N128(.fungible), name128::from_number(get_sym_id(2)), var.get_object(), key_seeds, payer), fungible_cannot_update_exception);

    // restore back
    my_tester->control->get_execution_context().set_version_unsafe(N(updfungible), 1);
    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "issuefungible_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "address": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "number" : "12.00000 S#1",
      "memo": "memo"
    }
    )=====";

    auto  var     = fc::json::from_string(test_data);
    auto  issfg   = var.as<issuefungible>();
    auto& tokendb = my_tester->control->token_db();
    CHECK(!EXISTS_ASSET(key, symbol(5, get_sym_id())));

    issfg.number = asset::from_string(string("15000.00000 S#") + std::to_string(get_sym_id()));
    to_variant(issfg, var);

    //issue rft more than balance exception
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), fungible_supply_exception);

    issfg.number  = asset::from_string(string("5000.00000 S#") + std::to_string(get_sym_id()));
    issfg.address = key;
    to_variant(issfg, var);

    issfg.address = address();
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    issfg.address = address(".abc", "test", 123);
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    issfg.number  = asset::from_string(string("5000.000000 S#") + std::to_string(get_sym_id()));
    issfg.address = address(key);
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible),(name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), asset_symbol_exception);

    issfg.number  = asset::from_string(string("5000.00000 S#") + std::to_string(get_sym_id()));
    to_variant(issfg, var); 
    my_tester->push_action(N(issuefungible), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    issfg.number = asset::from_string(string("15.00000 S#333"));
    to_variant(issfg, var);
    CHECK_THROWS_AS(my_tester->push_action(N(issuefungible), N128(.fungible),(name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), action_authorize_exception);

    asset ast;
    READ_DB_ASSET(key, symbol(5, get_sym_id()), ast);
    CHECK(5000'00000 == ast.amount());

    issfg.number = asset::from_string(string("15.00000 S#1"));
    to_variant(issfg, var);

    signed_transaction trx;
    trx.actions.emplace_back(my_tester->get_action(N(issuefungible), N128(.fungible), N128(1), var.get_object()));
    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(const auto& auth : key_seeds) {
        trx.sign(my_tester->get_private_key(auth), my_tester->control->get_chain_id());
    }
    trx.sign(fc::crypto::private_key(std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")), my_tester->control->get_chain_id());
    my_tester->push_transaction(trx);

    READ_DB_ASSET(issfg.address, symbol(5, 1), ast);
    CHECK(15'00000 == ast.amount());

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "transferft_test", "[contracts]") {
    const char* test_data = R"=====(
    {
      "from": "jmzk6NPexVQjcb2FJZJohZHsQ22rRRtHziH8yPfyj2zwnJV74Ycp2p",
      "to": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "number" : "12.00000 S#0",
      "memo": "memo"
    }
    )=====";

    auto var    = fc::json::from_string(test_data);
    auto trft   = var.as<transferft>();
    trft.number = asset::from_string(string("15000.00000 S#") + std::to_string(get_sym_id()));
    trft.from   = key;
    trft.to     = address(tester::get_public_key(N(to)));
    to_variant(trft, var);

    //transfer rft more than balance exception
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), balance_exception);

    trft.to = address();
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    trft.to = address(".abc", "test", 123);
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    trft.to     = address(tester::get_public_key(N(to)));
    trft.number = asset::from_string(string("15.000000 S#") + std::to_string(get_sym_id()));
    to_variant(trft, var);
    key_seeds.push_back(N(to));
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), asset_symbol_exception);


    trft.number = asset::from_string(string("15.00000 S#") + std::to_string(get_sym_id()));
    to_variant(trft, var);
    my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    auto payer2 = address(N(fungible), name128::from_number(get_sym_id()), 0);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2), payer_exception);

    payer2 = address(N(.fungible), name128::from_number(get_sym_id()), 0);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2), charge_exceeded_exception);

    my_tester->add_money(payer2, asset(100'000'000, jmzk_sym()));
    my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer2);

    auto& tokendb = my_tester->control->token_db();
    asset ast;
    READ_DB_ASSET(address(tester::get_public_key(N(to))), symbol(5, get_sym_id()), ast);
    CHECK(30'00000 == ast.amount());

    //from == to test
    trft.from = address(tester::get_public_key(N(to)));
    to_variant(trft, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer), fungible_address_exception);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "recycleft_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "address": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
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
    
    property ast_from_before;
    property ast_to_before;
    READ_DB_ASSET(rf.address, symbol(5, get_sym_id()), ast_from_before);
    READ_DB_ASSET_NO_THROW(fungible_address, symbol(5, get_sym_id()), ast_to_before);

    my_tester->push_action(N(recycleft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    property ast_from_after;
    property ast_to_after;
    READ_DB_ASSET(rf.address, symbol(5, get_sym_id()), ast_from_after);
    READ_DB_ASSET(fungible_address, symbol(5, get_sym_id()), ast_to_after);
    CHECK(100000 == ast_from_before.amount - ast_from_after.amount);
    CHECK(100000 == ast_to_after.amount - ast_to_before.amount);
}

TEST_CASE_METHOD(contracts_test, "destroyft_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "address": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
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

    property ast_from_before;
    property ast_to_before;
    READ_DB_ASSET(rf.address, symbol(5, get_sym_id()), ast_from_before);
    READ_DB_ASSET_NO_THROW(address(), symbol(5, get_sym_id()), ast_to_before);

    my_tester->push_action(N(destroyft), N128(.fungible), (name128)std::to_string(get_sym_id()), var.get_object(), key_seeds, payer);

    property ast_from_after;
    property ast_to_after;
    READ_DB_ASSET(rf.address, symbol(5, get_sym_id()), ast_from_after);
    READ_DB_ASSET(address(), symbol(5, get_sym_id()), ast_to_after);
    CHECK(100000 == ast_from_before.amount - ast_from_after.amount);
    CHECK(100000 == ast_to_after.amount - ast_to_before.amount);
}

TEST_CASE_METHOD(contracts_test, "jmzk2pjmzk_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "from": "jmzk6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
        "to": "jmzk548LviBDF6EcknKnKUMeaPUrZN2uhfCB1XrwHsURZngakYq9Vx",
        "number": "5.00000 S#4",
        "memo": "memo"
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  e2p     = var.as<jmzk2pjmzk>();
    auto& tokendb = my_tester->control->token_db();

    e2p.from = payer;
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(jmzk2pjmzk), N128(.fungible), (name128)std::to_string(jmzk_sym().id()), var.get_object(), key_seeds, payer), asset_symbol_exception);

    e2p.number = asset::from_string(string("5.00000 S#1"));
    e2p.to = address();
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(jmzk2pjmzk), N128(.fungible), (name128)std::to_string(jmzk_sym().id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    e2p.to = address(".hi", "test", 123);
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(jmzk2pjmzk), N128(.fungible), (name128)std::to_string(jmzk_sym().id()), var.get_object(), key_seeds, payer), address_reserved_exception);

    e2p.number = asset::from_string(string("5.000000 S#1"));
    e2p.to = key;
    to_variant(e2p, var);
    CHECK_THROWS_AS(my_tester->push_action(N(jmzk2pjmzk), N128(.fungible), (name128)std::to_string(jmzk_sym().id()), var.get_object(), key_seeds, payer), asset_symbol_exception);

    e2p.number = asset::from_string(string("5.00000 S#1"));
    to_variant(e2p, var);
    my_tester->push_action(N(jmzk2pjmzk), N128(.fungible), (name128)std::to_string(jmzk_sym().id()), var.get_object(), key_seeds, payer);

    asset ast;
    READ_DB_ASSET(key, pjmzk_sym(), ast);
    CHECK(500000 == ast.amount());

    auto tf   = var.as<transferft>();
    tf.from   = key;
    tf.to     = payer;
    tf.number = asset(50, symbol(5,2));

    to_variant(tf, var);
    CHECK_THROWS_AS(my_tester->push_action(N(transferft), N128(.fungible), (name128)std::to_string(pjmzk_sym().id()), var.get_object(), key_seeds, payer), unsatisfied_authorization);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "group_addr_test", "[contracts]") {
    // create a new group
    auto test_group = fmt::format(R"=====(
    {{
      "name" : "ajxtest",
      "group" : {{
        "name": "ajxtest",
        "key": "{}",
        "root": {{
          "threshold": 6,
          "weight": 0,
          "nodes": [{{
              "key": "{}",
              "weight": 2
            }},{{
              "key": "{}",
              "weight": 3
            }},{{
              "key": "{}",
              "weight": 2
            }}
          ]
        }}
      }}
    }}
    )=====",
    (std::string)tester::get_public_key("gkey"),
    (std::string)tester::get_public_key("gkey1"),
    (std::string)tester::get_public_key("gkey2"),
    (std::string)tester::get_public_key("gkey3"));

    auto gvar = fc::json::from_string(test_group);
    my_tester->push_action(N(newgroup), N128(.group), "ajxtest", gvar.get_object(), { N(gkey), N(payer) }, payer);

    // get group addresses
    auto addr1 = address(".group", "ajxtest", 0);
    auto addr2 = address(".group", "ajxtest", 123);

    auto tf   = transferft();
    tf.from   = payer;
    tf.to     = addr1;
    tf.number = asset(1000, jmzk_sym());
    tf.memo   = "hello";

    // transfer to group address
    CHECK_NOTHROW(my_tester->push_action(action(".fungible", "1", tf), key_seeds, payer));

    // cannot transfer FT without group authorized
    tf.from = addr1;
    tf.to   = poorer;
    CHECK_THROWS_AS(my_tester->push_action(action(".fungible", "1", tf), key_seeds, payer), unsatisfied_authorization);

    // transfer with group authorized, not enough authorizers
    CHECK_THROWS_AS(my_tester->push_action(action(".fungible", "1", tf), { N(gkey), N(payer) }, payer), unsatisfied_authorization);
    CHECK_THROWS_AS(my_tester->push_action(action(".fungible", "1", tf), { N(gkey1), N(gkey2), N(payer) }, payer), unsatisfied_authorization);

    // enough authorizers
    CHECK_NOTHROW(my_tester->push_action(action(".fungible", "1", tf), { N(gkey1), N(gkey2), N(gkey3), N(payer) }, payer));
}
