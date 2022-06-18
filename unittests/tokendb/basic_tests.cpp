#include "tokendb_tests.hpp"

const char* domain_data = R"=====(
    {
      "name" : "domain",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
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
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      },
      "metas":[{
      	"key": "key",
      	"value": "value",
      	"creator": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
      }]
    }
    )=====";

const char* token_data = R"=====(
    {
      	"domain": "domain",
        "name": "t1",
        "owner": [
          "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
        ],
        "metas":[{
      	"key": "key",
      	"value": "value",
      	"creator": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
      }]
    }
    )=====";

const char* fungible_data = R"=====(
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
      "total_supply":"100.00000 S#3"
    }
    )=====";

TEST_CASE_METHOD(tokendb_test, "add_token_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    // token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-test";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    
    ADD_TOKEN(domain, dom.name, dom); 
    CHECK(EXISTS_TOKEN(domain, dom.name));

    // token_type : token
    var = fc::json::from_string(token_data);
    auto tk = var.as<token_def>();
    tk.domain = "dm-tkdb-test";
    tk.owner[0] = key;
    CHECK(!EXISTS_TOKEN2(token, tk.domain, tk.name));
    
    ADD_TOKEN2(token, dom.name, tk.name, tk);
    
    CHECK(EXISTS_TOKEN2(token, tk.domain, tk.name));
}

TEST_CASE_METHOD(tokendb_test, "upd_token_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    auto dom = domain_def();
    auto tk = token_def();
    
    // token_type : domain(non-token)
    CHECK(EXISTS_TOKEN(domain, "dm-tkdb-test"));
    READ_TOKEN(domain, "dm-tkdb-test", dom);
    CHECK(dom.metas[0].key == "key");
    dom.metas[0].key = "key-tkdb-test";
    //dom.metas[0].key = "ky";
    
    UPDATE_TOKEN(domain, "dm-tkdb-test", dom);
    
    READ_TOKEN(domain, "dm-tkdb-test", dom);
    CHECK(dom.metas[0].key == "key-tkdb-test");
    //CHECK(dom.metas[0].key == "ky");

    // token_type : token
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "t1"));
    READ_TOKEN2(token, "dm-tkdb-test", "t1", tk);
    CHECK(tk.metas[0].key == "key");
    tk.metas[0].key = "key-tkdb-test";
    
    UPDATE_TOKEN2(token, "dm-tkdb-test", "t1", tk);
    
    READ_TOKEN2(token, "dm-tkdb-test", "t1", tk);
    CHECK(tk.metas[0].key == "key-tkdb-test");
}

TEST_CASE_METHOD(tokendb_test, "put_token_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    // * put_token: add a token
    
    // ** token_type : domain(non-token)
    auto var = fc::json::from_string(domain_data);
    auto dom = var.as<domain_def>();
    dom.creator = key;
    dom.name = "dm-tkdb-test1";
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(domain, dom.name));
    PUT_TOKEN(domain, dom.name, dom);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    // ** token_type : token
    var = fc::json::from_string(token_data);
    auto tk = var.as<token_def>();
    tk.domain = dom.name;
    tk.name = "t2";
    CHECK(!EXISTS_TOKEN2(token, dom.name, tk.name));
    PUT_TOKEN2(token, dom.name, tk.name, tk);
    CHECK(EXISTS_TOKEN2(token, dom.name, tk.name));

    // * put_token: upd a token
     
    // ** token_type : domain(non-token)
    auto _dom = domain_def();
    READ_TOKEN(domain, "dm-tkdb-test1", _dom);
    CHECK(_dom.metas[0].key == "key");
    _dom.metas[0].key = "key-tkdb-test1";
    PUT_TOKEN(domain, "dm-tkdb-test1", _dom);
    READ_TOKEN(domain, "dm-tkdb-test1", _dom);
    CHECK(_dom.metas[0].key == "key-tkdb-test1");

    // ** token_type : token
    auto _tk = token_def();
    READ_TOKEN2(token, "dm-tkdb-test1", "t2", _tk);
    CHECK(_tk.metas[0].key == "key");
    _tk.metas[0].key = "t2-meta";
    PUT_TOKEN2(token, _tk.domain, _tk.name, _tk);
    READ_TOKEN2(token, "dm-tkdb-test1", "t2", _tk);
    CHECK(_tk.metas[0].key == "t2-meta");
}

TEST_CASE_METHOD(tokendb_test, "put_asset_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    // add a new fungible for test
    auto var = fc::json::from_string(fungible_data);
    auto fg = var.as<fungible_def>();
    fg.creator = key;
    fg.issue.authorizers[0].ref.set_account(key);
    fg.manage.authorizers[0].ref.set_account(key);
    CHECK(!EXISTS_TOKEN(fungible, 3));
    PUT_TOKEN(fungible, 3, fg);
    CHECK(EXISTS_TOKEN(fungible, 3));

    // put asset
    auto addr = public_key_type(std::string("jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    auto as = asset::from_string("1.00000 S#3");
    CHECK(!EXISTS_ASSET(addr, 3));
    PUT_ASSET(addr, 3, as);
    CHECK(EXISTS_ASSET(addr, 3));
}

TEST_CASE_METHOD(tokendb_test, "put_tokens_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    CHECK(EXISTS_TOKEN(domain, "dm-tkdb-test"));
    
    auto var = fc::json::from_string(token_data);
    auto tk1 = var.as<token_def>();
    auto tk2 = tk1;
    tk1.domain = tk2.domain = "dm-tkdb-test";
    tk1.name = "basic-1";
    tk2.name = "basic-2";

    auto tkeys = jmzk::chain::token_keys_t();
    tkeys.push_back(tk1.name);
    tkeys.push_back(tk2.name);
    
    auto data = small_vector<std::string_view, 4>();
    data.push_back(jmzk::chain::make_db_value(tk1).as_string_view());
    data.push_back(jmzk::chain::make_db_value(tk2).as_string_view());

    tokendb.put_tokens(
            jmzk::chain::token_type::token,
            jmzk::chain::action_op::put,
            "dm-tkdb-test",
            std::move(tkeys),
            data);
    
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-1"));
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-2"));
}
