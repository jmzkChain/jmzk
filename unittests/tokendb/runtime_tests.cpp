#include "tokendb_tests.hpp"

const char* domain_data_rt = R"=====(
    {
      "name" : "domain-rt-test",
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

const char* token_data_rt = R"=====(
    {
      	"domain": "domain-rt-test",
        "name": "rt1",
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

const char* fungible_data_rt = R"=====(
    {
      "name": "ERT",
      "sym_name": "ERT",
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
      "total_supply":"100.00000 S#3"
    }
    )=====";

TEST_CASE_METHOD(tokendb_test, "add_token_svpt_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    my_tester->produce_block();
    ADD_SAVEPOINT;

    // token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data_rt);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    ADD_TOKEN(domain, dom.name, dom); 
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // token_type : token
    var = fc::json::from_string(token_data_rt);
    auto tk = var.as<token_def>();
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
    auto var = fc::json::from_string(domain_data_rt);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-rt1";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    ADD_SAVEPOINT;

    // ** token_type : token
    var = fc::json::from_string(token_data_rt);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "rt2";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));

    ADD_SAVEPOINT;

    // * put_token: upd a token
     
    // ** token_type : domain(non-token)
    auto _dom = domain_def();
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(_dom.metas[0].key == "key");
    _dom.metas[0].key = "key-tkdb-rt1";
    PUT_TOKEN(domain, "dm-tkdb-rt1", _dom);
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(_dom.metas[0].key == "key-tkdb-rt1");

    ADD_SAVEPOINT;

    // ** token_type : token
    auto _tk = token_def();
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(_tk.metas[0].key == "key");
    _tk.metas[0].key = "t2-meta";
    PUT_TOKEN2(token, _tk.domain, _tk.name, _tk);
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(_tk.metas[0].key == "t2-meta");

    ROLLBACK;
    READ_TOKEN2(token, "dm-tkdb-rt1", "rt2", _tk);
    CHECK(std::string(_tk.metas[0].key) == "key");
    
    ROLLBACK;
    READ_TOKEN(domain, "dm-tkdb-rt1", _dom);
    CHECK(std::string(_dom.metas[0].key) == "key");
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN2(token, "dm-tkdb-rt1", "t2"));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(domain, "dm-tkdb-rt1"));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_asset_svpt__test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();

    ADD_SAVEPOINT;
    
    // add a new fungible for test
    auto var = fc::json::from_string(fungible_data_rt);
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

    ROLLBACK;
    CHECK(!EXISTS_ASSET(addr, 4));
    
    ROLLBACK;
    CHECK(!EXISTS_TOKEN(fungible, 4));

    my_tester->produce_block();
}

TEST_CASE_METHOD(tokendb_test, "put_tokens_svpt_test", "[tokendb]") {
    CHECK(true);
}

TEST_CASE_METHOD(tokendb_test, "squansh_test", "[tokendb]") {
    CHECK(true);
}
