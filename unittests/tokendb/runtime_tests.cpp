#include "tokendb_tests.hpp"

TEST_CASE_METHOD(tokendb_test, "add_token_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    my_tester->produce_block();
    ADD_SAVEPOINT;

    // token_type : domain(non-token)
    auto var = fc::json::from_string(upddomain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-test";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    ADD_TOKEN(domain, dom.name, dom); 
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // token_type : token
    var = fc::json::from_string(updtoken_data);
    auto tk = var.as<token_def>();
    tk.domain = "dm-tkdb-test";
    tk.owner[0] = key;
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    ADD_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, tk.domain, tk.name));

    ROLLBACK;
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(domain, dom.name));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_token_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    ADD_SAVEPOINT;
    
    // * put_token: add a token
    
    // ** token_type : domain(non-token)
    auto var = fc::json::from_string(upddomain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-test1";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // ** token_type : token
    var = fc::json::from_string(updtoken_data);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "t2";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));

    ADD_SAVEPOINT;

    // * put_token: upd a token
     
    // ** token_type : domain(non-token)
    auto _dom = domain_def();
    READ_TOKEN(domain, "dm-tkdb-test1", _dom);
    CHECK(_dom.metas[0].key == "key");
    _dom.metas[0].key = "key-tkdb-test1";
    PUT_TOKEN(domain, "dm-tkdb-test1", _dom);
    READ_TOKEN(domain, "dm-tkdb-test1", _dom);
    CHECK(_dom.metas[0].key == "key-tkdb-test1");

    ADD_SAVEPOINT;

    // ** token_type : token
    auto _tk = token_def();
    READ_TOKEN2(token, "dm-tkdb-test1", "t2", _tk);
    CHECK(_tk.metas[0].key == "key");
    _tk.metas[0].key = "t2-meta";
    PUT_TOKEN2(token, _tk.domain, _tk.name, _tk);
    READ_TOKEN2(token, "dm-tkdb-test1", "t2", _tk);
    CHECK(_tk.metas[0].key == "t2-meta");

    ROLLBACK;
    READ_TOKEN2(token, "dm-tkdb-test1", "t2", _tk);
    CHECK(std::string(_tk.metas[0].key) == "key");
    
    ROLLBACK;
    READ_TOKEN(domain, "dm-tkdb-test1", _dom);
    CHECK(std::string(_dom.metas[0].key) == "key");
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN2(token, "dm-tkdb-test1", "t2"));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(domain, "dm-tkdb-test1"));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_asset_svpt__test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();

    ADD_SAVEPOINT;
    
    // add a new fungible for test
    auto var = fc::json::from_string(newfungible_data);
    auto fg = var.as<fungible_def>();
    fg.creator = key;
    fg.issue.authorizers[0].ref.set_account(key);
    fg.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(fungible, 3));
    PUT_TOKEN(fungible, 3, fg);
    CHECK(EXISTS_TOKEN(fungible, 3));

    ADD_SAVEPOINT;

    // put asset
    auto addr = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    auto as = asset::from_string("1.00000 S#3");
    CHECK(!EXISTS_ASSET(addr, 3));
    PUT_ASSET(addr, 3, as);
    CHECK(EXISTS_ASSET(addr, 3));

    ROLLBACK;
    CHECK(!EXISTS_ASSET(addr, 3));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(fungible, 3));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_tokens_test", "[tokendb]") {
    CHECK(true);
}

TEST_CASE_METHOD(tokendb_test, "squansh_test", "[tokendb]") {
    CHECK(true);
}
