#include "contracts_tests.hpp"

auto CHECK_EQUAL = [](auto& lhs, auto& rhs) {
    auto b1 = fc::raw::pack(lhs);
    auto b2 = fc::raw::pack(rhs);

    CHECK(b1.size() == b2.size());
    CHECK(memcmp(b1.data(), b2.data(), b1.size()) == 0);
};

TEST_CASE_METHOD(contracts_test, "newnftlock_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "proposer": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
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
                    "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                    "jmzk8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
                ]
            }
        },
        "succeed": [
        ],
        "failed": [
            "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        ]
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  nl      = var.as<newlock>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();
    CHECK(!EXISTS_TOKEN(lock, nl.name));

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

    nl.succeed = {public_key_type(std::string("jmzk8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))};
    to_variant(nl, var);

    my_tester->push_action(N(newlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    CHECK(EXISTS_TOKEN(lock, nl.name));

    lock_def lock_;
    READ_TOKEN(lock, nl.name, lock_);
    CHECK(lock_.status == lock_status::proposed);

    token_def tk;
    READ_TOKEN2(token, get_domain_name(), "t3", tk);
    CHECK(tk.owner.size() == 1);
    CHECK(tk.owner[0] == address(N(.lock), N128(nlact.name), 0));

    my_tester->produce_blocks();

    READ_TOKEN2(token, N128(.lock), N128(nftlock), lock_);
    auto lock2_ = cache.read_token<lock_def>(token_type::token, N128(.lock), N128(nftlock));
    CHECK(lock2_ != nullptr);
    CHECK_EQUAL(lock_, *lock2_);
}

TEST_CASE_METHOD(contracts_test, "newftlock_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "name": "ftlock",
        "proposer": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "unlock_time": "2020-06-09T09:06:27",
        "deadline": "2020-07-09T09:06:27",
        "assets": [{
            "type": "fungible",
            "data": {
                "from": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                "amount": "5.00000 S#2"
            }
        }],
        "condition": {
            "type": "cond_keys",
            "data": {
                "threshold": 3,
                "cond_keys": [
                    "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                    "jmzk8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
                ]
            }
        },
        "succeed": [
        ],
        "failed": [
            "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        ]
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  nl      = var.as<newlock>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();
    CHECK(!EXISTS_TOKEN(lock, nl.name));

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

    nl.succeed = {public_key_type(std::string("jmzk8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))};
    to_variant(nl, var);
    my_tester->push_action(N(newlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000);

    CHECK(EXISTS_TOKEN(lock, nl.name));

    lock_def lock_;
    READ_TOKEN(lock, nl.name, lock_);
    CHECK(lock_.status == lock_status::proposed);

    asset ast;
    READ_DB_ASSET(address(N(.lock), N128(nlact.name), 0), nl.assets[0].get<lockft_def>().amount.sym(), ast);
    CHECK(ast.amount() == 500000);

    my_tester->produce_blocks();

    READ_TOKEN2(token, N128(.lock), N128(ftlock), lock_);
    auto lock2_ = cache.read_token<lock_def>(token_type::token, N128(.lock), N128(ftlock));
    CHECK(lock2_ != nullptr);
    CHECK_EQUAL(lock_, *lock2_);
}

TEST_CASE_METHOD(contracts_test, "aprvlock_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "approver": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
        "data": {
            "type": "cond_key",
            "data": {}
        }
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  al      = var.as<aprvlock>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();

    lock_def lock_;
    READ_TOKEN(lock, al.name, lock_);
    CHECK(lock_.signed_keys.size() == 0);

    CHECK_THROWS_AS(my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    al.approver = {tester::get_public_key(N(payer))};
    to_variant(al, var);
    CHECK_THROWS_AS(my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_aprv_data_exception);

    al.approver = {tester::get_public_key(N(key))};
    to_variant(al, var);
    
    my_tester->push_action(N(aprvlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    READ_TOKEN(lock, al.name, lock_);
    CHECK(lock_.signed_keys.size() == 1);

    my_tester->produce_blocks();

    READ_TOKEN2(token, N128(.lock), N128(nftlock), lock_);
    auto lock2_ = cache.read_token<lock_def>(token_type::token, N128(.lock), N128(nftlock));
    CHECK(lock2_ != nullptr);
    CHECK_EQUAL(lock_, *lock2_);
}

TEST_CASE_METHOD(contracts_test, "tryunlock_test", "[contracts]") {
    const char* test_data = R"=======(
    {
        "name": "nftlock",
        "executor": "jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
    }
    )=======";

    auto  var     = fc::json::from_string(test_data);
    auto  tul     = var.as<tryunlock>();
    auto& tokendb = my_tester->control->token_db();
    auto& cache = my_tester->control->token_db_cache();

    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), unsatisfied_authorization);

    tul.executor = {tester::get_public_key(N(key))};
    to_variant(tul, var);
    
    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_not_reach_unlock_time);

    my_tester->produce_block();
    my_tester->produce_block(fc::days(12));
    my_tester->push_action(N(tryunlock), N128(.lock), N128(nftlock), var.get_object(), key_seeds, payer, 5'000'000);

    lock_def lock_;
    READ_TOKEN(lock, tul.name, lock_);
    CHECK(lock_.status == lock_status::succeed);

    token_def tk;
    READ_TOKEN2(token, get_domain_name(), "t3", tk);
    CHECK(tk.owner.size() == 1);
    CHECK(tk.owner[0] == public_key_type(std::string("jmzk8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC")));

    tul.name = N128(ftlock);
    to_variant(tul, var);

    // not reach deadline
    // not all conditional keys are signed
    CHECK_THROWS_AS(my_tester->push_action(N(tryunlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000), lock_not_reach_deadline);

    lock_def ft_lock;
    READ_TOKEN(lock, N128(ftlock), ft_lock);
    CHECK(ft_lock.status == lock_status::proposed);

    my_tester->produce_block();
    my_tester->produce_block(fc::days(12));

    // exceed deadline, turn into failed
    my_tester->push_action(N(tryunlock), N128(.lock), N128(ftlock), var.get_object(), key_seeds, payer, 5'000'000);

    READ_TOKEN(lock, N128(ftlock), ft_lock);
    CHECK(ft_lock.status == lock_status::failed);

    // failed address
    asset ast;
    READ_DB_ASSET(address(public_key_type(std::string("jmzk7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"))), symbol(5, get_sym_id()), ast);
    CHECK(ast.amount() == 500000);

    my_tester->produce_blocks();

    READ_TOKEN2(token, N128(.lock), N128(nftlock), lock_);
    auto lock2_ = cache.read_token<lock_def>(token_type::token, N128(.lock), N128(nftlock));
    CHECK(lock2_ != nullptr);
    CHECK_EQUAL(lock_, *lock2_);
}
