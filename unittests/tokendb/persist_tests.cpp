#include "tokendb_tests.hpp"

const char* domain_data_ps = R"=====(
    {
      "name" : "domain-ps-test",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
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
            "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      },
      "metas":[{
      	"key": "key",
      	"value": "value",
      	"creator": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
      }]
    }
    )=====";

const char* token_data_ps = R"=====(
    {
      	"domain": "domain-ps-test",
        "name": "ps1",
        "owner": [
          "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
        ],
        "metas":[{
      	"key": "key",
      	"value": "value",
      	"creator": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
      }]
    }
    )=====";

const char* fungible_data_ps = R"=====(
    {
      "name": "EPS",
      "sym_name": "EPS",
      "sym": "5,S#5",
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

/*
 * Persist Tests: add token
 */
TEST_CASE_METHOD(tokendb_test, "add_token_prst_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    my_tester->produce_block();
    ADD_SAVEPOINT;

    // token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data_ps);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    ADD_TOKEN(domain, dom.name, dom); 
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // token_type : token
    var = fc::json::from_string(token_data_ps);
    auto tk = var.as<token_def>();
    tk.owner[0] = key;
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    ADD_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, tk.domain, tk.name));

    ADD_SAVEPOINT;
}

TEST_CASE_METHOD(tokendb_test, "add_token_prst_rlbk", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    CHECK(EXISTS_TOKEN(domain, "domain-ps-test"));
    CHECK(EXISTS_TOKEN2(token, "domain-ps-test", "ps1"));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN2(token, "domain-ps-test", "ps1"));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(domain, "domain-ps-test"));

    ROLLBACK;
}
    

/*
 * Persist Tests: put token
 */
TEST_CASE_METHOD(tokendb_test, "put_token_prst_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    ADD_SAVEPOINT;
    
    // * put_token: add a token
    
    // ** token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data_ps);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-ps1";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // ** token_type : token
    var = fc::json::from_string(token_data_ps);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "ps2";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));

    ADD_SAVEPOINT;

    // * put_token: upd a token
     
    // ** token_type : domain(non-token)
    auto _dom = domain_def();
    READ_TOKEN(domain, "dm-tkdb-ps1", _dom);
    CHECK(_dom.metas[0].key == "key");
    _dom.metas[0].key = "key-tkdb-ps1";
    PUT_TOKEN(domain, "dm-tkdb-ps1", _dom);
    READ_TOKEN(domain, "dm-tkdb-ps1", _dom);
    CHECK(_dom.metas[0].key == "key-tkdb-ps1");

    ADD_SAVEPOINT;

    // ** token_type : token
    auto _tk = token_def();
    READ_TOKEN2(token, "dm-tkdb-ps1", "ps2", _tk);
    CHECK(_tk.metas[0].key == "key");
    _tk.metas[0].key = "ps2-meta";
    PUT_TOKEN2(token, _tk.domain, _tk.name, _tk);
    READ_TOKEN2(token, "dm-tkdb-ps1", "ps2", _tk);
    CHECK(_tk.metas[0].key == "ps2-meta");

    ADD_SAVEPOINT;
}

TEST_CASE_METHOD(tokendb_test, "put_token_prst_rlbk", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    auto _dom = domain_def();
    auto _tk = token_def();

    ROLLBACK;
    READ_TOKEN2(token, "dm-tkdb-ps1", "ps2", _tk);
    CHECK(std::string(_tk.metas[0].key) == "key");
    
    ROLLBACK;
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(std::string(_dom.metas[0].key) == "key");

    ROLLBACK;
    CHECK(!EXISTS_TOKEN2(token, "dm-tkdb-ps1", "ps2"));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(domain, "dm-tkdb-ps1"));

    ROLLBACK;
}

/*
 * Persist Tests: put asset
 */
TEST_CASE_METHOD(tokendb_test, "put_asset_prst_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();

    ADD_SAVEPOINT;
    
    // add a new fungible for test
    auto var = fc::json::from_string(fungible_data_ps);
    auto fg = var.as<fungible_def>();
    fg.creator = key;
    fg.issue.authorizers[0].ref.set_account(key);
    fg.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(fungible, 4));
    PUT_TOKEN(fungible, 4, fg);
    CHECK(EXISTS_TOKEN(fungible, 4));

    ADD_SAVEPOINT;

    // put asset
    auto addr = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    auto as = asset::from_string("1.00000 S#4");
    CHECK(!EXISTS_ASSET(addr, 4));
    PUT_ASSET(addr, 4, as);
    CHECK(EXISTS_ASSET(addr, 4));

    ADD_SAVEPOINT;
}

TEST_CASE_METHOD(tokendb_test, "put_asset_prst_rlbk", "[tokendb") {
    auto& tokendb = my_tester->control->token_db();
    auto addr = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));

    ROLLBACK;
    CHECK(!EXISTS_ASSET(addr, 4));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(fungible, 4));

    ROLLBACK;
}

/*
 * Persist Tests: squash
 */
TEST_CASE_METHOD(tokendb_test, "squash_prst_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    
    ADD_SAVEPOINT;
    
    auto var = fc::json::from_string(domain_data_ps);
    auto dom = var.as<domain_def>();
    dom.name = "domain-prst-sq";
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;
    
    var = fc::json::from_string(token_data_ps);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "ps-sq";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));
    
    ADD_SAVEPOINT;
    
    auto n = tokendb.savepoints_size();
    
    ADD_SAVEPOINT;
    ADD_SAVEPOINT;
    tokendb.squash();
    tokendb.squash();
    CHECK(tokendb.savepoints_size() == n);
    
}

TEST_CASE_METHOD(tokendb_test, "squash_prst_rlbk", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    CHECK(EXISTS_TOKEN2(token, "domain-prst-sq", "ps-sq"));
    CHECK(EXISTS_TOKEN(domain, "domain-prst-sq"));

    tokendb.squash();

    CHECK(EXISTS_TOKEN2(token, "domain-prst-sq", "ps-sq"));
    CHECK(EXISTS_TOKEN(domain, "domain-prst-sq"));
    
    tokendb.squash();

    CHECK(EXISTS_TOKEN2(token, "domain-prst-sq", "ps-sq"));
    CHECK(EXISTS_TOKEN(domain, "domain-prst-sq"));

    //Only one savepoint left now.
    ROLLBACK;
    
    CHECK(!EXISTS_TOKEN2(token, "domain-prst-sq", "ps-sq"));
    CHECK(!EXISTS_TOKEN(domain, "domain-prst-sq"));
}
    
TEST_CASE_METHOD(tokendb_test, "put_tokens_prst_test", "[tokendb]") {
    CHECK(true);
}
