#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <iterator>
#include <vector>

#include <catch/catch.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <fc/variant.hpp>

#include <evt/chain/controller.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;

extern std::string evt_unittests_dir;

class tokendb_test {
public:
    tokendb_test() : tokendb(evt_unittests_dir + "/tokendb_tests") {
        tokendb.open();
    }
    ~tokendb_test() {}

protected:
    int32_t
    get_time() {
        return time(0) + (++ti);
    }

protected:
    token_database tokendb;
    static int     ti;
};

int tokendb_test::ti = 0;

domain_def
add_domain_data() {
    const char* test_data = R"=====(
    {
      "name" : "domain",
      "creator" : "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "create_time":"2018-06-09T09:06:27",
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
      }
    }
    )=====";

    auto var = fc::json::from_string(test_data);
    auto dom = var.as<domain_def>();
    return dom;
}

domain_def
update_domain_data() {
    const char* test_data = R"=====(
    {
      "name" : "domain",
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

    auto var = fc::json::from_string(test_data);
    auto dom = var.as<domain_def>();
    return dom;
}

issuetoken
issue_tokens_data() {
    const char* test_data = R"=====(
    {
      	"domain": "domain",
        "names": [
          "t1",
          "t2"
        ],
        "owner": [
          "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
        ]
    }
    )=====";

    auto var  = fc::json::from_string(test_data);
    auto istk = var.as<issuetoken>();
    return istk;
}

token_def
update_token_data() {
    const char* test_data = R"=====(
    {
      	"domain": "domain",
        "name": "t1",
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

    auto var = fc::json::from_string(test_data);
    auto tk  = var.as<token_def>();
    return tk;
}

group_def
add_group_data() {
    const char* test_data = R"=====(
    {
		"name": "group",
		"key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		"root": {
		  "threshold": 6,
		  "weight": 0,
		  "nodes": [{
		      "type": "branch",
		      "threshold": 1,
		      "weight": 3,
		      "nodes": [{
		          "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		          "weight": 1
		        },{
		          "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		          "weight": 1
		        }
		      ]
		    },{
		      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		      "weight": 3
		    },{
		      "threshold": 1,
		      "weight": 3,
		      "nodes": [{
		          "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		          "weight": 1
		        },{
		          "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		          "weight": 2
		        }
		      ]
		    }
		  ]
		}
	}
    )=====";

    auto var = fc::json::from_string(test_data);
    auto gp  = var.as<group_def>();
    return gp;
}

group_def
update_group_data() {
    const char* test_data = R"=====(
    {
		"name": "group",
		"key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		"root": {
		  "threshold": 5,
		  "weight": 0,
		  "nodes": [{
		      "type": "branch",
		      "threshold": 1,
		      "weight": 3,
		      "nodes": [{
		          "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		          "weight": 1
		        },{
		          "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		          "weight": 1
		        }
		      ]
		    },{
		      "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		      "weight": 3
		    },{
		      "threshold": 1,
		      "weight": 3,
		      "nodes": [{
		          "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
		          "weight": 1
		        },{
		          "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
		          "weight": 2
		        }
		      ]
		    }
		  ]
		}
	}
    )=====";

    auto var = fc::json::from_string(test_data);
    auto gp  = var.as<group_def>();
    return gp;
}

suspend_def
add_suspend_data() {
    const char* test_data = R"=======(
        {
            "name": "testsuspend",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "status": "proposed",
            "trx": {
                "expiration": "2018-07-04T05:14:12",
                "ref_block_num": "3432",
                "ref_block_prefix": "291678901",
                "actions": [
                    {
                        "name": "newdomain",
                        "domain": "test1530681222",
                        "key": ".create",
                        "data": "00000000004010c4a02042710c9f077d0002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000008052e74c01000000010100000002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000000000000100000000b298e982a40100000001020000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000094135c6801000000010100000002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000000000000100"
                    }
                ],
                "transaction_extensions": []
            },
            "signed_keys": [],
            "signatures": []
        }
        )=======";

    auto var = fc::json::from_string(test_data);
    auto dl  = var.as<suspend_def>();
    return dl;
}

suspend_def
update_suspend_data() {
    const char* test_data = R"=======(
        {
            "name": "testsuspend",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "status": "executed",
            "trx": {
                "expiration": "2018-07-04T05:14:12",
                "ref_block_num": "3432",
                "ref_block_prefix": "291678901",
                "actions": [
                    {
                        "name": "newdomain",
                        "domain": "test1530681222",
                        "key": ".create",
                        "data": "00000000004010c4a02042710c9f077d0002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000008052e74c01000000010100000002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000000000000100000000b298e982a40100000001020000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000094135c6801000000010100000002e07ae3ed523dba04dc9d718d94abcd1bea3da38176f4b775b818200c01a149b1000000000000000100"
                    }
                ],
                "transaction_extensions": []
            },
            "signed_keys": [],
            "signatures": []
        }
        )=======";

    auto var = fc::json::from_string(test_data);
    auto dl  = var.as<suspend_def>();
    return dl;
}

lock_def 
add_lock_data() {
    const char* test_data = R"=======(
        {
            "name": "testsuspend",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "status": "proposed",
            "unlock_time": "2018-07-04T05:14:12",
            "deadline": "2018-09-04T05:14:12",
            "assets": [{
                "type": "tokens",
                "tokens": {
                    "domain": "cookie",
                    "names": [
                        "t1",
                        "t2",
                        "t3"
                    ]
                }
            }],
            "cond_keys": [
                "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
            ],
            "succeed": [
                "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
            ],
            "failed": [
                "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
            ],
            "signed_keys": [
            ]
        }
        )=======";

    auto var = fc::json::from_string(test_data);
    auto al  = var.as<lock_def>();
    return al;
}

lock_def 
update_lock_data() {
    const char* test_data = R"=======(
        {
            "name": "testsuspend",
            "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
            "status": "succeed",
            "unlock_time": "2018-07-04T05:14:12",
            "deadline": "2018-09-04T05:14:12",
            "assets": [{
                "type": "tokens",
                "tokens": {
                    "domain": "cookie",
                    "names": [
                        "t1",
                        "t2",
                        "t3"
                    ]
                }
            }],
            "cond_keys": [
                "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
            ],
            "succeed": [
                "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
            ],
            "failed": [
                "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
            ],
            "signed_keys": [
                "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
            ]
        }
        )=======";

    auto var = fc::json::from_string(test_data);
    auto ul  = var.as<lock_def>();
    return ul;
}

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(evt::chain::token_type::##TYPE, std::nullopt, NAME)

#define ADD_TOKEN(TYPE, NAME, VALUEREF) \
    { \
        auto dbvalue = evt::chain::make_db_value(VALUEREF); \
        tokendb.add_token(evt::chain::token_type::##TYPE, evt::chain::action_op::add, std::nullopt, dbvalue.as_string_view()); \
    }

#define UPDATE_TOKEN(TYPE, NAME, VALUEREF) \
    { \
        auto dbvalue = evt::chain::make_db_value(VALUEREF); \
        tokendb.add_token(evt::chain::token_type::##TYPE, evt::chain::action_op::update, std::nullopt, dbvalue.as_string_view()); \
    }

#define READ_TOKEN(TYPE, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(evt::chain::token_type::##TYPE, std::nullopt, NAME, str); \
        evt::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_TOKEN2(TYPE, DOMAIN, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(evt::chain::token_type::##TYPE, DOMAIN, NAME, str); \
        evt::chain::extract_db_value(str, VALUEREF); \
    }

TEST_CASE_METHOD(tokendb_test, "tokendb_adddomain_test", "[tokendb]") {
    CHECK(true);

    auto dom = add_domain_data();
    CHECK(!EXISTS_TOKEN(domain, dom.name));

    auto re = ADD_TOKEN(domain, dom.name, dom);
    REQUIRE(re == 0);
    CHECK(EXISTS_TOKEN(domain, dom.name));

    domain_def dom_;
    READ_TOKEN(domain, dom.name, dom_);
    CHECK(dom.name == dom_.name);
    CHECK(dom.create_time.to_iso_string() == dom_.create_time.to_iso_string());

    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.creator);

    CHECK("issue" == dom_.issue.name);
    CHECK(1 == dom_.issue.threshold);
    REQUIRE(1 == dom_.issue.authorizers.size());
    CHECK(dom_.issue.authorizers[0].ref.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issue.authorizers[0].ref.get_account());
    CHECK(1 == dom_.issue.authorizers[0].weight);

    CHECK("transfer" == dom_.transfer.name);
    CHECK(1 == dom_.transfer.threshold);
    REQUIRE(1 == dom_.transfer.authorizers.size());
    CHECK(dom_.transfer.authorizers[0].ref.is_owner_ref());
    CHECK(1 == dom_.transfer.authorizers[0].weight);

    CHECK("manage" == dom_.manage.name);
    CHECK(1 == dom_.manage.threshold);
    REQUIRE(1 == dom_.manage.authorizers.size());
    CHECK(dom_.manage.authorizers[0].ref.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.manage.authorizers[0].ref.get_account());
    CHECK(1 == dom_.manage.authorizers[0].weight);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updatedomain_test", "[tokendb]") {
    domain_def dom = update_domain_data();
    REQUIRE(EXISTS_TOKEN(domain, dom.name));
    dom.metas[0].key = "key" + boost::lexical_cast<std::string>(time(0));

    auto re = UPDATE_TOKEN(domain, dom.name, dom);
    REQUIRE(re == 0);

    domain_def dom_;
    READ_TOKEN(domain, dom.name, dom_);

    CHECK(dom.name == dom_.name);

    CHECK("issue" == dom_.issue.name);
    CHECK(1 == dom_.issue.threshold);
    REQUIRE(1 == dom_.issue.authorizers.size());
    CHECK(dom_.issue.authorizers[0].ref.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issue.authorizers[0].ref.get_account());
    CHECK(1 == dom_.issue.authorizers[0].weight);

    CHECK("transfer" == dom_.transfer.name);
    CHECK(1 == dom_.transfer.threshold);
    REQUIRE(1 == dom_.transfer.authorizers.size());
    CHECK(dom_.transfer.authorizers[0].ref.is_owner_ref());
    CHECK(1 == dom_.transfer.authorizers[0].weight);

    CHECK("manage" == dom_.manage.name);
    CHECK(1 == dom_.manage.threshold);
    REQUIRE(1 == dom_.manage.authorizers.size());
    CHECK(dom_.manage.authorizers[0].ref.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.manage.authorizers[0].ref.get_account());
    CHECK(1 == dom_.manage.authorizers[0].weight);

    REQUIRE(1 == dom_.metas.size());
    CHECK(dom.metas[0].key == dom_.metas[0].key);
    CHECK("value" == dom_.metas[0].value);
    CHECK(dom_.metas[0].creator.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.metas[0].creator.get_account());
}

TEST_CASE_METHOD(tokendb_test, "tokendb_issuetoken_test", "[tokendb]") {
    issuetoken istk = issue_tokens_data();
    CHECK(!tokendb.exists_token(istk.domain, istk.names[0]));
    CHECK(!tokendb.exists_token(istk.domain, istk.names[1]));

    auto re = tokendb.issue_tokens(istk);
    REQUIRE(re == 0);

    CHECK(tokendb.exists_token(istk.domain, istk.names[0]));
    CHECK(tokendb.exists_token(istk.domain, istk.names[1]));

    token_def tk1_;
    token_def tk2_;
    READ_TOKEN(token, istk.domain, istk.names[0], tk1_);

    CHECK("domain" == tk1_.domain);
    CHECK(istk.names[0] == tk1_.name);
    CHECK(istk.owner == tk1_.owner);

    READ_TOKEN(token, istk.domain, istk.names[1], tk2_);

    CHECK("domain" == tk2_.domain);
    CHECK(istk.names[1] == tk2_.name);
    CHECK(istk.owner == tk2_.owner);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updatetoken_test", "[tokendb]") {
    token_def tk    = update_token_data();
    tk.metas[0].key = "key" + boost::lexical_cast<std::string>(time(0));

    auto re = tokendb.update_token(tk);
    REQUIRE(re == 0);

    token_def tk_;
    READ_TOKEN(token, tk.domain, tk.name, tk_);

    CHECK("domain" == tk_.domain);
    CHECK(tk.name == tk_.name);
    CHECK(tk.owner == tk_.owner);

    REQUIRE(1 == tk_.metas.size());
    CHECK(tk.metas[0].key == tk_.metas[0].key);
    CHECK("value" == tk_.metas[0].value);
    CHECK(tk_.metas[0].creator.is_account_ref());
    CHECK("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)tk_.metas[0].creator.get_account());
}

TEST_CASE_METHOD(tokendb_test, "tokendb_addgroup_test", "[tokendb]") {
    group_def gp = add_group_data();
    CHECK(!tokendb.exists_group(gp.name_));

    auto re = tokendb.add_group(gp);
    REQUIRE(re == 0);
    CHECK(tokendb.exists_group(gp.name_));

    group_def gp_;
    READ_TOKEN(group, gp.name(), gp_);

    CHECK(gp.name() == gp_.name());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.key());

    auto root = gp_.root();
    REQUIRE(root.validate());
    REQUIRE(root.is_root());
    REQUIRE(3 == root.size);
    CHECK(1 == root.index);
    CHECK(6 == root.threshold);
    CHECK(0 == root.weight);

    auto son0 = gp_.get_child_node(root, 0);
    REQUIRE(son0.validate());
    REQUIRE(2 == son0.size);
    CHECK(1 == son0.threshold);
    CHECK(3 == son0.weight);

    auto son0_son0 = gp_.get_child_node(son0, 0);
    REQUIRE(son0_son0.validate());
    REQUIRE(son0_son0.is_leaf());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son0_son0));
    CHECK(1 == son0_son0.weight);

    auto son0_son1 = gp_.get_child_node(son0, 1);
    REQUIRE(son0_son1.validate());
    REQUIRE(son0_son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son0_son1));
    CHECK(1 == son0_son1.weight);

    auto son1 = gp_.get_child_node(root, 1);
    REQUIRE(son1.validate());
    REQUIRE(son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son1));
    CHECK(3 == son1.weight);

    auto son2 = gp_.get_child_node(root, 2);
    REQUIRE(son2.validate());
    REQUIRE(2 == son2.size);
    CHECK(1 == son2.threshold);
    CHECK(3 == son2.weight);

    auto son2_son0 = gp_.get_child_node(son2, 0);
    REQUIRE(son2_son0.validate());
    REQUIRE(son2_son0.is_leaf());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son2_son0));
    CHECK(1 == son2_son0.weight);

    auto son2_son1 = gp_.get_child_node(son2, 1);
    REQUIRE(son2_son1.validate());
    REQUIRE(son2_son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son2_son1));
    CHECK(2 == son2_son1.weight);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updategroup_test", "[tokendb]") {
    group_def gp = update_group_data();
    auto      re = tokendb.update_group(gp);

    REQUIRE(re == 0);
    CHECK(tokendb.exists_group(gp.name_));

    group_def gp_;
    READ_TOKEN(group, gp.name(), gp_);

    CHECK(gp.name() == gp_.name());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.key());

    auto root = gp_.root();
    REQUIRE(root.validate());
    REQUIRE(root.is_root());
    REQUIRE(3 == root.size);
    CHECK(1 == root.index);
    CHECK(5 == root.threshold);
    CHECK(0 == root.weight);

    auto son0 = gp_.get_child_node(root, 0);
    REQUIRE(son0.validate());
    REQUIRE(2 == son0.size);
    CHECK(1 == son0.threshold);
    CHECK(3 == son0.weight);

    auto son0_son0 = gp_.get_child_node(son0, 0);
    REQUIRE(son0_son0.validate());
    REQUIRE(son0_son0.is_leaf());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son0_son0));
    CHECK(1 == son0_son0.weight);

    auto son0_son1 = gp_.get_child_node(son0, 1);
    REQUIRE(son0_son1.validate());
    REQUIRE(son0_son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son0_son1));
    CHECK(1 == son0_son1.weight);

    auto son1 = gp_.get_child_node(root, 1);
    REQUIRE(son1.validate());
    REQUIRE(son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son1));
    CHECK(3 == son1.weight);

    auto son2 = gp_.get_child_node(root, 2);
    REQUIRE(son2.validate());
    REQUIRE(2 == son2.size);
    CHECK(1 == son2.threshold);
    CHECK(3 == son2.weight);

    auto son2_son0 = gp_.get_child_node(son2, 0);
    REQUIRE(son2_son0.validate());
    REQUIRE(son2_son0.is_leaf());
    CHECK("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son2_son0));
    CHECK(1 == son2_son0.weight);

    auto son2_son1 = gp_.get_child_node(son2, 1);
    REQUIRE(son2_son1.validate());
    REQUIRE(son2_son1.is_leaf());
    CHECK("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son2_son1));
    CHECK(2 == son2_son1.weight);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_fungible_test", "[tokendb]") {
    auto tmp_fungible = fungible_def();

    CHECK(!tokendb.exists_fungible(EVT_SYM_ID));
    CHECK(!tokendb.exists_fungible(symbol(5, EVT_SYM_ID)));
    CHECK_THROWS_AS(READ_TOKEN(fungible, EVT_SYM_ID, tmp_fungible), unknown_fungible_exception);
    CHECK_THROWS_AS(READ_TOKEN(fungible, symbol(5, EVT_SYM_ID), tmp_fungible), unknown_fungible_exception);

    auto evt_fungible = fungible_def();
    evt_fungible.sym  = symbol(5, EVT_SYM_ID);
    auto r            = tokendb.add_fungible(evt_fungible);
    CHECK(r == 0);

    CHECK(tokendb.exists_fungible(EVT_SYM_ID));
    CHECK(tokendb.exists_fungible(symbol(5, EVT_SYM_ID)));
    CHECK(tokendb.exists_fungible(symbol(4, EVT_SYM_ID)));

    CHECK_NOTHROW(READ_TOKEN(fungible, EVT_SYM_ID, tmp_fungible));
    CHECK(tmp_fungible.sym == symbol(5, EVT_SYM_ID));
    CHECK_NOTHROW(READ_TOKEN(fungible, symbol(5, EVT_SYM_ID), tmp_fungible));
    CHECK(tmp_fungible.sym == symbol(5, EVT_SYM_ID));

    auto tmp_asset = asset();
    auto address1  = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    CHECK(!tokendb.exists_any_asset(address1));
    CHECK(!tokendb.exists_asset(address1, symbol(5, EVT_SYM_ID)));
    CHECK_THROWS_AS(READ_TOKEN(asset, address1, symbol(5, EVT_SYM_ID), tmp_asset), balance_exception);
    CHECK_NOTHROW(READ_TOKEN(asset_no_throw, address1, symbol(5, EVT_SYM_ID), tmp_asset));
    CHECK(tmp_asset == asset(0, symbol(5, EVT_SYM_ID)));

    int ETH = 666;
    auto s = 0;
    READ_TOKEN(all_assets, address1, [&](const auto&) { s++; return true; });
    CHECK(s == 0);

    auto r1 = tokendb.update_asset(address1, asset(2000, symbol(5, EVT_SYM_ID)));
    auto r2 = tokendb.update_asset(address1, asset(1000, symbol(8, ETH)));

    CHECK(r1 == 0);
    CHECK(r2 == 0);

    CHECK(tokendb.exists_any_asset(address1));
    CHECK(tokendb.exists_asset(address1, symbol(5, EVT_SYM_ID)));
    CHECK(tokendb.exists_asset(address1, symbol(8, ETH)));
    CHECK(!tokendb.exists_asset(address1, symbol(4, EVT_SYM_ID)));
    CHECK_NOTHROW(READ_TOKEN(asset, address1, symbol(5, EVT_SYM_ID), tmp_asset));
    CHECK(tmp_asset == asset(2000, symbol(5, EVT_SYM_ID)));

    auto s2 = 0;
    READ_TOKEN(all_assets, address1, [&](const auto& s) { INFO((std::string)s); s2++; return true; });
    CHECK(s2 == 2);

    auto address2 = address(N(domain), "domain", 0);
    READ_TOKEN(all_assets, address2, [&](const auto&) { s++; return true; });
    CHECK(s == 0);

    r1 = tokendb.update_asset(address2, asset(2000, symbol(5, EVT_SYM_ID)));
    r2 = tokendb.update_asset(address2, asset(1000, symbol(8, ETH)));

    CHECK(r1 == 0);
    CHECK(r2 == 0);

    CHECK(tokendb.exists_any_asset(address2));
    CHECK(tokendb.exists_asset(address2, symbol(5, EVT_SYM_ID)));
    CHECK(tokendb.exists_asset(address2, symbol(8, ETH)));
    CHECK(!tokendb.exists_asset(address2, symbol(4, EVT_SYM_ID)));
    CHECK_NOTHROW(READ_TOKEN(asset, address2, symbol(5, EVT_SYM_ID), tmp_asset));
    CHECK(tmp_asset == asset(2000, symbol(5, EVT_SYM_ID)));

    s2 = 0;
    READ_TOKEN(all_assets, address2, [&](const auto& s) { INFO((std::string)s); s2++; return true; });
    CHECK(s2 == 2);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_checkpoint_test", "[tokendb]") {
    tokendb.add_savepoint(get_time());

    domain_def dom = add_domain_data();
    dom.name       = "domain-" + boost::lexical_cast<std::string>(time(0));
    tokendb.add_domain(dom);
    tokendb.add_savepoint(get_time());

    domain_def updom = update_domain_data();
    updom.name       = dom.name;
    tokendb.update_domain(updom);
    tokendb.add_savepoint(get_time());

    issuetoken istk = issue_tokens_data();
    istk.domain     = dom.name;
    tokendb.issue_tokens(istk);
    tokendb.add_savepoint(get_time());

    token_def tk = update_token_data();
    tk.domain    = dom.name;
    tokendb.update_token(tk);

    REQUIRE(tokendb.exists_token(dom.name, "t1"));
    token_def tk_;
    READ_TOKEN(token, dom.name, "t1", tk_);
    REQUIRE(1 == tk_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(token, dom.name, "t1", tk_);
    CHECK(0 == tk_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!tokendb.exists_token(dom.name, "t1"));

    REQUIRE(EXISTS_TOKEN(domain, dom.name));
    domain_def dom_;
    READ_TOKEN(domain, dom.name, dom_);
    REQUIRE(1 == dom_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(domain, dom.name, dom_);
    REQUIRE(0 == dom_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!EXISTS_TOKEN(domain, dom.name));

    tokendb.add_savepoint(get_time());
    group_def gp = add_group_data();
    gp.name_     = "group-" + boost::lexical_cast<std::string>(time(0));
    tokendb.add_group(gp);
    tokendb.add_savepoint(get_time());

    group_def upgp = update_group_data();
    upgp.name_     = gp.name();
    tokendb.update_group(upgp);

    REQUIRE(tokendb.exists_group(gp.name()));
    group_def gp_;
    READ_TOKEN(group, gp.name(), gp_);
    auto root = gp_.root();
    CHECK(5 == root.threshold);
    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(group, gp.name(), gp_);
    root = gp_.root();
    CHECK(6 == root.threshold);
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!tokendb.exists_group(gp.name()));

    tokendb.add_savepoint(get_time());
    gp       = add_group_data();
    gp.name_ = "group--" + boost::lexical_cast<std::string>(time(0));
    tokendb.add_group(gp);

    tokendb.add_savepoint(get_time());
    upgp       = update_group_data();
    upgp.name_ = gp.name();
    tokendb.update_group(upgp);

    int pop_re = tokendb.pop_savepoints(get_time());
    REQUIRE(pop_re == 0);

    tokendb.add_savepoint(get_time());
    auto pevt    = symbol(5, PEVT_SYM_ID);
    auto address = public_key_type((std::string) "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV");
    CHECK(!tokendb.exists_fungible(PEVT_SYM_ID));
    CHECK(!tokendb.exists_any_asset(address));
    CHECK(!tokendb.exists_asset(address, pevt));

    fungible_def fungible;
    fungible.sym = symbol(5, EVT_SYM_ID);
    tokendb.add_fungible(fungible);
    tokendb.update_asset(address, asset(1000, pevt));

    CHECK(tokendb.exists_fungible(EVT_SYM_ID));
    CHECK(tokendb.exists_asset(address, pevt));

    tokendb.add_savepoint(get_time());
    tokendb.update_asset(address, asset(2000, pevt));

    tokendb.rollback_to_latest_savepoint();

    asset a;
    READ_TOKEN(asset, address, pevt, a);
    CHECK(a == asset(1000, pevt));

    tokendb.add_savepoint(get_time());
    evt_link_object link_obj;
    link_obj.link_id = 111;
    tokendb.add_evt_link(link_obj);

    auto r = tokendb.rollback_to_latest_savepoint();
    CHECK(r == 0);

    CHECK(!tokendb.exists_evt_link(111));

    r = tokendb.rollback_to_latest_savepoint();
    CHECK(r == 0);

    CHECK(!tokendb.exists_fungible(PEVT_SYM_ID));
    CHECK(!tokendb.exists_any_asset(address));
    CHECK(!tokendb.exists_asset(address, pevt));
    
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    CHECK_NOTHROW(tokendb.pop_savepoints(time(0) + ti + 1));

    CHECK(tokendb.savepoints_size() == 0);
    {
        auto ss1 = tokendb.new_savepoint_session();
        CHECK(ss1.seq() == 1);
        tokendb.update_asset(address, asset(2000, pevt));
        CHECK(tokendb.exists_any_asset(address));
    }
    CHECK(!tokendb.exists_any_asset(address));
    CHECK(tokendb.savepoints_size() == 0);

    tokendb.add_savepoint(get_time());
    tokendb.update_asset(address, asset(2000, pevt));

    {
        auto ss1 = tokendb.new_savepoint_session();
        CHECK(ss1.seq() == time(0) + ti + 1);
        tokendb.update_asset(address, asset(4000, pevt));
        ss1.accept();
    }

    READ_TOKEN(asset, address, pevt, a);
    CHECK(a == asset(4000, pevt));
    CHECK(tokendb.savepoints_size() == 2);

    {
        auto ss1 = tokendb.new_savepoint_session();
        CHECK(ss1.seq() == time(0) + ti + 2);
        tokendb.update_asset(address, asset(6000, pevt));
        ss1.squash();
    }
    READ_TOKEN(asset, address, pevt, a);
    CHECK(a == asset(6000, pevt));
    CHECK(tokendb.savepoints_size() == 2);

    CHECK_NOTHROW(tokendb.pop_savepoints(0));

    tokendb.pop_savepoints(get_time() + 100);
    CHECK(tokendb.savepoints_size() == 0);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_addsuspend_test", "[tokendb]") {
    CHECK(tokendb.savepoints_size() == 0);

    auto dl = add_suspend_data();
    CHECK(!tokendb.exists_suspend(dl.name));

    auto re = tokendb.add_suspend(dl);
    REQUIRE(re == 0);
    CHECK(tokendb.exists_suspend(dl.name));

    suspend_def dl_;
    READ_TOKEN(suspend, dl.name, dl_);

    CHECK(suspend_status::proposed == dl_.status);
    CHECK(dl.name == dl_.name);
    CHECK("EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3" == (std::string)dl_.proposer);
    CHECK("2018-07-04T05:14:12" == dl_.trx.expiration.to_iso_string());
    CHECK(3432 == dl_.trx.ref_block_num);
    CHECK(291678901 == dl_.trx.ref_block_prefix);
    CHECK(dl_.trx.actions.size() == 1);
    CHECK("newdomain" == dl_.trx.actions[0].name);
    CHECK("test1530681222" == dl_.trx.actions[0].domain);
    CHECK(".create" == dl_.trx.actions[0].key);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updatesuspend_test", "[tokendb]") {
    CHECK(true);
    tokendb.add_savepoint(get_time());
    tokendb.rollback_to_latest_savepoint();
    auto dl = update_suspend_data();

    auto re = tokendb.update_suspend(dl);
    REQUIRE(re == 0);

    suspend_def dl_;
    READ_TOKEN(suspend, dl.name, dl_);

    CHECK(suspend_status::executed == dl_.status);
    CHECK(dl.name == dl_.name);
    CHECK("EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3" == (std::string)dl_.proposer);
    CHECK("2018-07-04T05:14:12" == dl_.trx.expiration.to_iso_string());
    CHECK(3432 == dl_.trx.ref_block_num);
    CHECK(291678901 == dl_.trx.ref_block_prefix);
    CHECK(dl_.trx.actions.size() == 1);
    CHECK("newdomain" == dl_.trx.actions[0].name);
    CHECK("test1530681222" == dl_.trx.actions[0].domain);
    CHECK(".create" == dl_.trx.actions[0].key);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updateprodvote_test", "[tokendb]") {
    CHECK(true);

    tokendb.add_savepoint(get_time());
    
    conf_key        key = "voter";
    public_key_type pkey1((std::string) "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");
    public_key_type pkey2((std::string) "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX");

    std::map<public_key_type, int> vote_sum;
    READ_TOKEN(prodvotes_no_throw, key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[pkey1] == 0);
    CHECK(vote_sum[pkey2] == 0);
    vote_sum.clear();

    auto re = tokendb.update_prodvote(key, pkey1, 1);
    REQUIRE(re == 0);

    READ_TOKEN(prodvotes_no_throw, key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[pkey1] == 1);
    CHECK(vote_sum[pkey2] == 0);
    vote_sum.clear();

    tokendb.rollback_to_latest_savepoint();

    tokendb.add_savepoint(get_time());

    re = tokendb.update_prodvote(key, pkey2, 2);
    REQUIRE(re == 0);

    READ_TOKEN(prodvotes_no_throw, key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[pkey1] == 0);
    CHECK(vote_sum[pkey2] == 2);
    vote_sum.clear();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_prodvote_presist_test", "[tokendb]") {
    tokendb.rollback_to_latest_savepoint();

    conf_key        key = "voter";
    public_key_type pkey1((std::string) "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");
    public_key_type pkey2((std::string) "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX");
    std::map<public_key_type, int> vote_sum;

    READ_TOKEN(prodvotes_no_throw, key, [&](const public_key_type& pkey, int votes) { vote_sum[pkey] += votes; return true; });
    CHECK(vote_sum[pkey1] == 0);
    CHECK(vote_sum[pkey2] == 0);
    vote_sum.clear();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_add_lock_test", "[tokendb]") {
    CHECK(tokendb.savepoints_size() == 0);
    tokendb.add_savepoint(get_time());

    auto al = add_lock_data();
    CHECK(!tokendb.exists_lock(al.name));

    auto re = tokendb.add_lock(al);
    REQUIRE(re == 0);
    CHECK(tokendb.exists_lock(al.name));

    lock_def al_;
    READ_TOKEN(lock, al.name, al_);

    CHECK(al.status == al_.status);
    CHECK(al.name == al_.name);
    CHECK((std::string)al.proposer == (std::string)al_.proposer);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_update_lock_test", "[tokendb]") {
    CHECK(tokendb.savepoints_size() == 1);
    tokendb.add_savepoint(get_time());

    auto ul = update_lock_data();

    auto re = tokendb.update_lock(ul);
    REQUIRE(re == 0);

    lock_def ul_;
    READ_TOKEN(lock, ul.name, ul_);

    CHECK(ul.status == ul_.status);
    CHECK(ul.name == ul_.name);
    CHECK((std::string)ul.proposer == (std::string)ul_.proposer);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_lock_presist_test", "[tokendb]") {
    CHECK(tokendb.savepoints_size() == 2);

    auto ul = update_lock_data();
    auto al = add_lock_data();

    CHECK(tokendb.exists_lock(al.name));
    lock_def lock_;
    READ_TOKEN(lock, ul.name, lock_);

    CHECK(lock_.status == ul.status);

    tokendb.rollback_to_latest_savepoint();

    CHECK(tokendb.exists_lock(al.name));
    READ_TOKEN(lock, ul.name, lock_);
    CHECK(lock_.status == al.status);

    tokendb.rollback_to_latest_savepoint();

    CHECK(!tokendb.exists_lock(al.name));

    CHECK(tokendb.savepoints_size() == 0);
}


TEST_CASE_METHOD(tokendb_test, "tokendb_squash", "[tokendb]") {
    CHECK(true);

    tokendb.add_savepoint(get_time());

    domain_def dom = add_domain_data();
    dom.name       = "domain-s1";
    tokendb.add_domain(dom);
    tokendb.add_savepoint(get_time());

    domain_def updom = update_domain_data();
    updom.name       = dom.name;
    tokendb.update_domain(updom);
    tokendb.add_savepoint(get_time());

    issuetoken istk = issue_tokens_data();
    istk.domain     = dom.name;
    tokendb.issue_tokens(istk);
    tokendb.add_savepoint(get_time());

    token_def tk = update_token_data();
    tk.domain    = dom.name;
    tokendb.update_token(tk);
    tokendb.add_savepoint(get_time());

    REQUIRE(tokendb.exists_token(dom.name, "t1"));
    token_def tk_;
    READ_TOKEN(token, dom.name, "t1", tk_);
    REQUIRE(1 == tk_.metas.size());

    auto n = tokendb.savepoints_size();

    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.squash();
    tokendb.squash();

    CHECK(tokendb.savepoints_size() == n);

    READ_TOKEN(token, dom.name, "t1", tk_);
    REQUIRE(1 == tk_.metas.size());
    REQUIRE(tokendb.exists_token(dom.name, "t1"));
    REQUIRE(EXISTS_TOKEN(domain, dom.name));

    tokendb.squash();
    tokendb.squash();
    tokendb.squash();
    tokendb.squash();

    CHECK(tokendb.savepoints_size() == 1);
    CHECK_THROWS_AS(tokendb.squash(), tokendb_squash_exception); // only one savepoint left
}

TEST_CASE_METHOD(tokendb_test, "tokendb_squash2", "[tokendb]") {
    CHECK(true);

    domain_def dom = add_domain_data();
    dom.name       = "domain-s1";

    REQUIRE(tokendb.exists_token(dom.name, "t1"));
    token_def tk_;
    READ_TOKEN(token, dom.name, "t1", tk_);
    REQUIRE(1 == tk_.metas.size());

    tokendb.rollback_to_latest_savepoint();

    REQUIRE(!tokendb.exists_token(dom.name, "t1"));
    REQUIRE(!EXISTS_TOKEN(domain, dom.name));
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_1", "[tokendb]") {
    CHECK(true);

    tokendb.add_savepoint(get_time());

    domain_def dom = add_domain_data();
    dom.name       = "domain-p1";
    tokendb.add_domain(dom);
    tokendb.add_savepoint(get_time());

    domain_def updom = update_domain_data();
    updom.name       = dom.name;
    tokendb.update_domain(updom);
    tokendb.add_savepoint(get_time());

    issuetoken istk = issue_tokens_data();
    istk.domain     = dom.name;
    tokendb.issue_tokens(istk);
    tokendb.add_savepoint(get_time());

    token_def tk = update_token_data();
    tk.domain    = dom.name;
    tokendb.update_token(tk);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_2", "[tokendb]") {
    CHECK(true);

    domain_def dom = add_domain_data();
    dom.name       = "domain-p1";

    REQUIRE(tokendb.exists_token(dom.name, "t1"));
    token_def tk_;
    READ_TOKEN(token, dom.name, "t1", tk_);
    REQUIRE(1 == tk_.metas.size());

    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(token, dom.name, "t1", tk_);
    CHECK(0 == tk_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!tokendb.exists_token(dom.name, "t1"));

    REQUIRE(EXISTS_TOKEN(domain, dom.name));
    domain_def dom_;
    READ_TOKEN(domain, dom.name, dom_);
    REQUIRE(1 == dom_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(domain, dom.name, dom_);
    REQUIRE(0 == dom_.metas.size());
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!EXISTS_TOKEN(domain, dom.name));

    tokendb.add_savepoint(get_time());
    group_def gp = add_group_data();
    gp.name_     = "group-p1";
    tokendb.add_group(gp);
    tokendb.add_savepoint(get_time());

    group_def upgp = update_group_data();
    upgp.name_     = gp.name();
    tokendb.update_group(upgp);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_3", "[tokendb]") {
    group_def gp = add_group_data();
    gp.name_     = "group-p1";

    REQUIRE(tokendb.exists_group(gp.name()));
    group_def gp_;
    READ_TOKEN(group, gp.name(), gp_);
    auto root = gp_.root();
    CHECK(5 == root.threshold);
    tokendb.rollback_to_latest_savepoint();
    READ_TOKEN(group, gp.name(), gp_);
    root = gp_.root();
    CHECK(6 == root.threshold);
    tokendb.rollback_to_latest_savepoint();
    REQUIRE(!tokendb.exists_group(gp.name()));

    tokendb.add_savepoint(get_time());
    gp       = add_group_data();
    gp.name_ = "group--" + boost::lexical_cast<std::string>(time(0));
    tokendb.add_group(gp);

    tokendb.add_savepoint(get_time());
    auto upgp  = update_group_data();
    upgp.name_ = gp.name();
    tokendb.update_group(upgp);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_4", "[tokendb]") {
    tokendb.pop_savepoints(get_time() + 1);

    int PPEVT = 777;

    tokendb.add_savepoint(get_time());
    auto pevt    = symbol(5, PPEVT);
    auto address = public_key_type((std::string) "EVT5tRjHNDPMxQfmejsGzNyQHRBiLAYEU7YZLfyHjvygnmmAUfYpX");
    CHECK(!tokendb.exists_fungible(PPEVT));
    CHECK(!tokendb.exists_any_asset(address));
    CHECK(!tokendb.exists_asset(address, pevt));

    fungible_def fungible;
    fungible.sym = symbol(5, EVT_SYM_ID);
    tokendb.add_fungible(fungible);
    tokendb.update_asset(address, asset(1000, pevt));

    CHECK(tokendb.exists_fungible(EVT_SYM_ID));
    CHECK(tokendb.exists_asset(address, pevt));

    tokendb.add_savepoint(get_time());
    tokendb.update_asset(address, asset(2000, pevt));
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_5", "[tokendb]") {
    int PPEVT = 777;
    
    tokendb.rollback_to_latest_savepoint();
    auto pevt    = symbol(5, PPEVT);
    auto address = public_key_type((std::string) "EVT5tRjHNDPMxQfmejsGzNyQHRBiLAYEU7YZLfyHjvygnmmAUfYpX");
    auto a       = asset();
    READ_TOKEN(asset, address, pevt, a);
    CHECK(a == asset(1000, pevt));

    auto r = tokendb.rollback_to_latest_savepoint();
    CHECK(r == 0);
    CHECK(!tokendb.exists_fungible(PPEVT));
    CHECK(!tokendb.exists_any_asset(address));
    CHECK(!tokendb.exists_asset(address, pevt));

    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
    tokendb.add_savepoint(get_time());
}

TEST_CASE_METHOD(tokendb_test, "tokendb_persist_savepoints_6", "[tokendb]") {
    CHECK_NOTHROW(tokendb.pop_savepoints(time(0) + ti + 1));
}
