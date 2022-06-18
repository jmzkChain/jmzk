#include "tokendb_tests.hpp"

TEST_CASE_METHOD(tokendb_test, "add_token_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    my_tester->produce_block();
    ADD_SAVEPOINT();

    // token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "domain-rt-test";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    ADD_TOKEN(domain, dom.name, dom); 
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT();

    // token_type : token
    var = fc::json::from_string(token_data);
    auto tk = var.as<token_def>();
    tk.owner[0] = key;
    tk.domain = dom.name;
    tk.name = "rt1";
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    ADD_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, tk.domain, tk.name));

    ROLLBACK();
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    
    ROLLBACK();
    CHECK(!EXISTS_TOKEN(domain, dom.name));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_token_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    ADD_SAVEPOINT();
    
    // * put_token: add a token
    
    // ** token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-rt1";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT();

    // ** token_type : token
    var = fc::json::from_string(token_data);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "rt2";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));

    ADD_SAVEPOINT();

    // * put_token: upd a token
     
    // ** token_type : domain(non-token)
    auto _dom = domain_def();
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(_dom.metas[0].key == "key");
    _dom.metas[0].key = "key-tkdb-rt1";
    PUT_TOKEN(domain, "dm-tkdb-rt1", _dom);
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(_dom.metas[0].key == "key-tkdb-rt1");

    ADD_SAVEPOINT();

    // ** token_type : token
    auto _tk = token_def();
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(_tk.metas[0].key == "key");
    _tk.metas[0].key = "t2-meta";
    PUT_TOKEN2(token, _tk.domain, _tk.name, _tk);
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(_tk.metas[0].key == "t2-meta");

    ROLLBACK();
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(std::string(_tk.metas[0].key) == "key");
    
    ROLLBACK();
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(std::string(_dom.metas[0].key) == "key");
    
    ROLLBACK();
    CHECK(!EXISTS_TOKEN2(token, "dm-tkdb-rt1", "t2"));
    
    ROLLBACK();
    CHECK(!EXISTS_TOKEN(domain, "dm-tkdb-rt1"));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_asset_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();

    ADD_SAVEPOINT();
    
    // add a new fungible for test
    auto var = fc::json::from_string(fungible_data);
    auto fg = var.as<fungible_def>();
    fg.creator = key;
    fg.issue.authorizers[0].ref.set_account(key);
    fg.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(fungible, 4));
    PUT_TOKEN(fungible, 4, fg);
    CHECK(EXISTS_TOKEN(fungible, 4));

    ADD_SAVEPOINT();

    // put asset
    auto addr = public_key_type(std::string("jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    auto as = asset::from_string("1.00000 S#4");
    CHECK(!EXISTS_ASSET(addr, 4));
    PUT_ASSET(addr, 4, as);
    CHECK(EXISTS_ASSET(addr, 4));

    ROLLBACK();
    CHECK(!EXISTS_ASSET(addr, 4));
    
    ROLLBACK();
    CHECK(!EXISTS_TOKEN(fungible, 4));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_tokens_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    ADD_SAVEPOINT();

    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "domain-rt-test";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    var = fc::json::from_string(token_data);
    auto tk1 = var.as<token_def>();
    auto tk2 = tk1;
    tk1.domain = tk2.domain = "domain-rt-test";
    tk1.name = "runtime-1";
    tk2.name = "runtime-2";

    auto tkeys = jmzk::chain::token_keys_t();
    tkeys.push_back(tk1.name);
    tkeys.push_back(tk2.name);

    auto data = small_vector<std::string_view, 4>();
    data.push_back(jmzk::chain::make_db_value(tk1).as_string_view());
    data.push_back(jmzk::chain::make_db_value(tk2).as_string_view());

    tokendb.put_tokens(
            jmzk::chain::token_type::token,
            jmzk::chain::action_op::put,
            "domain-rt-test",
            std::move(tkeys),
            data);

    CHECK(EXISTS_TOKEN2(token, "domain-rt-test", "runtime-1"));
    CHECK(EXISTS_TOKEN2(token, "domain-rt-test", "runtime-2"));

    ROLLBACK();
    
    CHECK(!EXISTS_TOKEN2(token, "domain-rt-test", "runtime-1"));
    CHECK(!EXISTS_TOKEN2(token, "domain-rt-test", "runtime-2"));
}

TEST_CASE_METHOD(tokendb_test, "squansh_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    
    ADD_SAVEPOINT();
    
    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.name = "domain-sq";
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT();
    
    var = fc::json::from_string(token_data);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "tk-sq";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));
    
    ADD_SAVEPOINT();
    
    auto n = tokendb.savepoints_size();
    
    ADD_SAVEPOINT();
    ADD_SAVEPOINT();
    tokendb.squash();
    tokendb.squash();
    
    CHECK(tokendb.savepoints_size() == n);
    
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));
    CHECK(EXISTS_TOKEN(domain, dom.name));

    tokendb.squash();
    tokendb.squash();
    tokendb.squash();

    ROLLBACK();
    
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    CHECK(!EXISTS_TOKEN(domain, dom.name));

    my_tester->produce_block();
}
