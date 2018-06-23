// #define BOOST_TEST_MODULE unittests
#define BOOST_TEST_DYN_LINK

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <time.h>
#include <vector>

#include <evt/chain/contracts/types.hpp>
#include <evt/chain/token_database.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <fc/variant.hpp>

#include <evt/chain/controller.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;

struct tokendb_test {
    tokendb_test() {
        tokendb.initialize("/tmp/tokendb1/");
        ti=0;
    }
    ~tokendb_test() {}
    int32_t get_time() {
    	return time(0)+(ti++);
    }
    token_database tokendb;
    int ti;
};

BOOST_FIXTURE_TEST_SUITE(tokendb_tests, tokendb_test)

BOOST_AUTO_TEST_CASE(tokendb_adddomain_test) {
    try {
        BOOST_CHECK(true);
        BOOST_TEST(tokendb.add_savepoint(get_time()) == 0);
        // BOOST_TEST(tokendb.rollback_to_latest_savepoint() == 0);
        // tokendb.add_savepoint(get_time());
        // tokendb.rollback_to_latest_savepoint();
        const char* test_data = R"=====(
	    {
	      "name" : "domain",
	      "issuer" : "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
	      "issue_time":"2018-06-09T09:06:27",
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
	            "ref": "[G] OWNER",
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

        auto       var = fc::json::from_string(test_data);
        domain_def dom = var.as<domain_def>();

       	if(tokendb.exists_domain(dom.name)) {
            dom.name = "name" + boost::lexical_cast<std::string>(time(0));
        }

        auto re = tokendb.add_domain(dom);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_domain(dom.name));

        domain_def dom_;
        tokendb.read_domain(dom.name, dom_);
        BOOST_TEST(dom.name == dom_.name);
        BOOST_TEST(dom.issue_time.to_iso_string() == dom_.issue_time.to_iso_string());

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issuer);

        BOOST_TEST("issue" == dom_.issue.name);
        BOOST_TEST(1 == dom_.issue.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.issue.authorizers.size());
        BOOST_TEST(dom_.issue.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issue.authorizers[0].ref.get_account());
        BOOST_TEST(1 == dom_.issue.authorizers[0].weight);

        BOOST_TEST("transfer" == dom_.transfer.name);
        BOOST_TEST(1 == dom_.transfer.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.transfer.authorizers.size());
        BOOST_TEST(dom_.transfer.authorizers[0].ref.is_owner_ref());
        BOOST_TEST(1 == dom_.transfer.authorizers[0].weight);

        BOOST_TEST("manage" == dom_.manage.name);
        BOOST_TEST(1 == dom_.manage.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.manage.authorizers.size());
        BOOST_TEST(dom_.manage.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.manage.authorizers[0].ref.get_account());
        BOOST_TEST(1 == dom_.manage.authorizers[0].weight);

       	tokendb.rollback_to_latest_savepoint();
    	BOOST_TEST(!tokendb.exists_domain(dom_.name));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updatedomain_test) {
    try {
        BOOST_CHECK(true);
        tokendb.add_savepoint(get_time());

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
	            "ref": "[G] OWNER",
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
	      "metas":[{
	      	"key": "key",
	      	"value": "value",
	      	"creator": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
	      }]
	    }
	    )=====";
	     
        auto       var = fc::json::from_string(test_data);
        domain_def dom = var.as<domain_def>();
        BOOST_TEST_REQUIRE(tokendb.exists_domain(dom.name));
        dom.metas[0].key = "key" +boost::lexical_cast<std::string>(time(0));
        auto re = tokendb.update_domain(dom);

        BOOST_TEST_REQUIRE(re == 0);

        domain_def dom_;
        tokendb.read_domain(dom.name, dom_);

        BOOST_TEST(dom.name == dom_.name);

        BOOST_TEST("issue" == dom_.issue.name);
        BOOST_TEST(1 == dom_.issue.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.issue.authorizers.size());
        BOOST_TEST(dom_.issue.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issue.authorizers[0].ref.get_account());
        BOOST_TEST(1 == dom_.issue.authorizers[0].weight);

        BOOST_TEST("transfer" == dom_.transfer.name);
        BOOST_TEST(1 == dom_.transfer.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.transfer.authorizers.size());
        BOOST_TEST(dom_.transfer.authorizers[0].ref.is_owner_ref());
        BOOST_TEST(1 == dom_.transfer.authorizers[0].weight);

        BOOST_TEST("manage" == dom_.manage.name);
        BOOST_TEST(1 == dom_.manage.threshold);
        BOOST_TEST_REQUIRE(1 == dom_.manage.authorizers.size());
        BOOST_TEST(dom_.manage.authorizers[0].ref.is_account_ref());
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.manage.authorizers[0].ref.get_account());
        BOOST_TEST(1 == dom_.manage.authorizers[0].weight);

        BOOST_TEST_REQUIRE(1 == dom_.metas.size());
        BOOST_TEST(dom.metas[0].key == dom_.metas[0].key);
        BOOST_TEST("value" == dom_.metas[0].value);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.metas[0].creator);

     	tokendb.rollback_to_latest_savepoint();
     	tokendb.read_domain(dom.name, dom_);
     	BOOST_TEST(dom.metas[0].key != dom_.metas[0].key);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_issuetoken_test) {
    try {
    	tokendb.add_savepoint(get_time());

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

        auto       var  = fc::json::from_string(test_data);
        issuetoken istk = var.as<issuetoken>();
        if(tokendb.exists_token(istk.domain, istk.names[0]))	istk.names[0]     = "t1" + boost::lexical_cast<std::string>(time(0));
        if(tokendb.exists_token(istk.domain, istk.names[1]))	istk.names[1]     = "t2" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.issue_tokens(istk);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[0]));
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[1]));

        token_def tk1_;
        token_def tk2_;
        tokendb.read_token(istk.domain, istk.names[0], tk1_);

        BOOST_TEST("domain" == tk1_.domain);
        BOOST_TEST(istk.names[0] == tk1_.name);
        BOOST_TEST(istk.owner == tk1_.owner);

        tokendb.read_token(istk.domain, istk.names[1], tk2_);

        BOOST_TEST("domain" == tk2_.domain);
        BOOST_TEST(istk.names[1] == tk2_.name);
        BOOST_TEST(istk.owner == tk2_.owner);

       	tokendb.rollback_to_latest_savepoint();
       	BOOST_TEST(!tokendb.exists_token("domain",istk.names[0]));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updatetoken_test) {
    try {
    	tokendb.add_savepoint(get_time());

        const char* test_data = R"=====(
	    {
	      	"domain": "domain",
	        "name": "t1",
	        "owner": [
	          "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
	        ]
	        "metas":[{
	      	"key": "key",
	      	"value": "value",
	      	"creator": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
	      }]
	    }
	    )=====";

        auto       var  = fc::json::from_string(test_data);
        token_def tk = var.as<token_def>();
        tk.metas[0].key = "key" +boost::lexical_cast<std::string>(time(0));
        auto re = tokendb.update_token(tk);

        BOOST_TEST_REQUIRE(re == 0);

        token_def tk_;
        tokendb.read_token(tk.domain, tk.name,tk_);

        BOOST_TEST("domain" == tk_.domain);
        BOOST_TEST(tk.name == tk_.name);
        BOOST_TEST(tk.owner == tk_.owner);

        BOOST_TEST_REQUIRE(1 == tk_.metas.size());
        BOOST_TEST(tk.metas[0].key == tk_.metas[0].key);
        BOOST_TEST("value" == tk_.metas[0].value);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)tk_.metas[0].creator);

        tokendb.rollback_to_latest_savepoint();
        tokendb.read_token(tk.domain, tk.name,tk_);
       	BOOST_TEST(tk.metas[0].key != tk_.metas[0].key);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(tokendb_addgroup_test) {
    try {
    	tokendb.add_savepoint(get_time());

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

        auto      var = fc::json::from_string(test_data);
        group_def gp  = var.as<group_def>();
    	if(tokendb.exists_group(gp.name_))	gp.name_       = "group" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.add_group(gp);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_group(gp.name_));

        group_def gp_;
        tokendb.read_group(gp.name(), gp_);

        BOOST_TEST(gp.name() == gp_.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.key());

        auto root = gp_.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(6 == root.threshold);
        BOOST_TEST(0 == root.weight);

        auto son0 = gp_.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        auto son0_son0 = gp_.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        auto son0_son1 = gp_.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        auto son1 = gp_.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        auto son2 = gp_.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        auto son2_son0 = gp_.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        auto son2_son1 = gp_.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);
        
        tokendb.rollback_to_latest_savepoint();
       	BOOST_TEST(!tokendb.exists_group(gp_.name()));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updategroup_test) {
    try {
    	tokendb.add_savepoint(get_time());

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

        auto      var = fc::json::from_string(test_data);
        group_def gp  = var.as<group_def>();
       
        auto re = tokendb.update_group(gp);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_group(gp.name_));

        group_def gp_;
        tokendb.read_group(gp.name(), gp_);

        BOOST_TEST(gp.name() == gp_.name());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.key());

        auto root = gp_.root();
        BOOST_TEST_REQUIRE(root.validate());
        BOOST_TEST_REQUIRE(root.is_root());
        BOOST_TEST_REQUIRE(3 == root.size);
        BOOST_TEST(1 == root.index);
        BOOST_TEST(5 == root.threshold);
        BOOST_TEST(0 == root.weight);

        auto son0 = gp_.get_child_node(root, 0);
        BOOST_TEST_REQUIRE(son0.validate());
        BOOST_TEST_REQUIRE(2 == son0.size);
        BOOST_TEST(1 == son0.threshold);
        BOOST_TEST(3 == son0.weight);

        auto son0_son0 = gp_.get_child_node(son0, 0);
        BOOST_TEST_REQUIRE(son0_son0.validate());
        BOOST_TEST_REQUIRE(son0_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son0_son0));
        BOOST_TEST(1 == son0_son0.weight);

        auto son0_son1 = gp_.get_child_node(son0, 1);
        BOOST_TEST_REQUIRE(son0_son1.validate());
        BOOST_TEST_REQUIRE(son0_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son0_son1));
        BOOST_TEST(1 == son0_son1.weight);

        auto son1 = gp_.get_child_node(root, 1);
        BOOST_TEST_REQUIRE(son1.validate());
        BOOST_TEST_REQUIRE(son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son1));
        BOOST_TEST(3 == son1.weight);

        auto son2 = gp_.get_child_node(root, 2);
        BOOST_TEST_REQUIRE(son2.validate());
        BOOST_TEST_REQUIRE(2 == son2.size);
        BOOST_TEST(1 == son2.threshold);
        BOOST_TEST(3 == son2.weight);

        auto son2_son0 = gp_.get_child_node(son2, 0);
        BOOST_TEST_REQUIRE(son2_son0.validate());
        BOOST_TEST_REQUIRE(son2_son0.is_leaf());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)gp_.get_leaf_key(son2_son0));
        BOOST_TEST(1 == son2_son0.weight);

        auto son2_son1 = gp_.get_child_node(son2, 1);
        BOOST_TEST_REQUIRE(son2_son1.validate());
        BOOST_TEST_REQUIRE(son2_son1.is_leaf());
        BOOST_TEST("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX" == (std::string)gp_.get_leaf_key(son2_son1));
        BOOST_TEST(2 == son2_son1.weight);
        
        tokendb.rollback_to_latest_savepoint();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_addaccount_test) {
    try {
    	tokendb.add_savepoint(get_time());

        const char* test_data = R"=====(
	    {
          "name": "account",
          "creator": "creator",
          "create_time":"2018-06-09T09:06:27",
          "balance": "12.00000 EVT",
          "frozen_balance": "12.00000 EVT",
          "owner": ["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
        }
	    )=====";

        auto      var = fc::json::from_string(test_data);
        account_def acct  = var.as<account_def>();
        if(tokendb.exists_account(acct.name))	acct.name = "account" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.add_account(acct);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_account(acct.name));

        account_def acct_;
        tokendb.read_account(acct.name, acct_);

        BOOST_TEST(acct.name == acct_.name);
        BOOST_TEST(acct.creator == acct_.creator);
        BOOST_TEST(acct.create_time.to_iso_string() == acct_.create_time.to_iso_string());

        BOOST_TEST(1200000 == acct_.balance.get_amount());
        BOOST_TEST("5,EVT" == acct_.balance.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == acct_.balance.to_string());
        BOOST_TEST(1200000 == acct_.frozen_balance.get_amount());
        BOOST_TEST("5,EVT" == acct_.frozen_balance.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == acct_.frozen_balance.to_string());

        BOOST_TEST_REQUIRE(1 == acct_.owner.size());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)acct_.owner[0]);
 		
 		tokendb.rollback_to_latest_savepoint();
 		BOOST_TEST(!tokendb.exists_account(acct_.name));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updateaccount_test) {
    try {
    	tokendb.add_savepoint(get_time());

        const char* test_data = R"=====(
	    {
          "name": "account",
          "creator": "creator",
          "create_time":"2018-06-09T09:06:27",
          "balance": "13.00000 EVT",
          "frozen_balance": "12.00000 EVT",
          "owner": ["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
        }
	    )=====";

        auto      var = fc::json::from_string(test_data);
        account_def acct  = var.as<account_def>();
        acct.creator = "creator" +boost::lexical_cast<std::string>(time(0));
        auto re = tokendb.update_account(acct);

        BOOST_TEST_REQUIRE(re == 0);
        
        account_def acct_;
        tokendb.read_account(acct.name, acct_);

        BOOST_TEST(acct.name == acct_.name);
        BOOST_TEST(acct.creator == acct_.creator);
        BOOST_TEST(acct.create_time.to_iso_string() == acct_.create_time.to_iso_string());

        BOOST_TEST(1300000 == acct_.balance.get_amount());
        BOOST_TEST("5,EVT" == acct_.balance.get_symbol().to_string());
        BOOST_TEST("13.00000 EVT" == acct_.balance.to_string());
        BOOST_TEST(1200000 == acct_.frozen_balance.get_amount());
        BOOST_TEST("5,EVT" == acct_.frozen_balance.get_symbol().to_string());
        BOOST_TEST("12.00000 EVT" == acct_.frozen_balance.to_string());

        BOOST_TEST_REQUIRE(1 == acct_.owner.size());
        BOOST_TEST("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" == (std::string)acct_.owner[0]);
 		
 		tokendb.rollback_to_latest_savepoint();
 		tokendb.read_account(acct.name, acct_);
 		BOOST_TEST(acct.creator != acct_.creator);
    }
    FC_LOG_AND_RETHROW()
}

// BOOST_AUTO_TEST_CASE(tokendb_adddelay_test) {
//     try {
//         const char* test_data = R"=====(
// 	    {
//           "name": "account",
//           "creator": "creator",
//           "create_time":"2018-06-09T09:06:27",
//           "balance": "12.00000 EVT",
//           "frozen_balance": "12.00000 EVT",
//           "owner": ["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
//         }
// 	    )=====";

//         auto      var = fc::json::from_string(test_data);
//         delay_def dly  = var.as<delay_def>();
//         dly.name      = "account" + boost::lexical_cast<std::string>(time(0));

//         auto re = tokendb.add_delay(acct);

//         BOOST_TEST_REQUIRE(re == 0);
//         BOOST_TEST(tokendb.exists_delay(acct.name));

//         tokendb.read_delay(acct.name, [&acct](const delay_def& acct_) {
	        
//         });
//     }
//     FC_LOG_AND_RETHROW()
// }

BOOST_AUTO_TEST_CASE(tokendb_checkpoint_test) {
    try {
    	// BOOST_TEST_REQUIRE(tokendb.exists_account("account"));
    	// account_def acct_;
     //    tokendb.read_account("account", acct_);
    	// BOOST_TEST(1300000 == acct_.balance.get_amount());
    	// tokendb.rollback_to_latest_savepoint();
    	// tokendb.read_account("account", acct_);
    	// BOOST_TEST(1200000 == acct_.balance.get_amount());
    	// tokendb.rollback_to_latest_savepoint();
    	// BOOST_TEST_REQUIRE(!tokendb.exists_account("account"));

    	// BOOST_TEST_REQUIRE(tokendb.exists_group("group"));
    	// group_def gp_;
     //    tokendb.read_account("group", acct_);
    	// auto root = gp_.root();
     //    BOOST_TEST(5 == root.threshold);
    	// tokendb.rollback_to_latest_savepoint();
    	// tokendb.read_account("group", acct_);
    	// root = gp_.root();
     //    BOOST_TEST(5 == root.threshold);
    	// tokendb.rollback_to_latest_savepoint();
    	// BOOST_TEST_REQUIRE(!tokendb.exists_group("group"));

    	// BOOST_TEST_REQUIRE(tokendb.exists_token("domain","t1"));
    	// token_def tk_;
     //    tokendb.read_token("domain","t1", tk_);
    	// BOOST_TEST_REQUIRE(1 == tk_.metas.size());
    	// tokendb.rollback_to_latest_savepoint();
    	// tokendb.read_token("domain","t1", tk_);
    	// BOOST_TEST_REQUIRE(0 == tk_.metas.size());
    	// tokendb.rollback_to_latest_savepoint();
    	// BOOST_TEST_REQUIRE(!tokendb.exists_token("domain","t1"));

    	// BOOST_TEST_REQUIRE(tokendb.exists_domain("domain"));
    	// domain_def dom_;
     //    tokendb.read_domain("domain",dom_);
    	// BOOST_TEST_REQUIRE(1 == dom_.metas.size());
    	// tokendb.rollback_to_latest_savepoint();
    	// tokendb.read_domain("domain",dom_);
    	// BOOST_TEST_REQUIRE(0 == dom_.metas.size());
    	// tokendb.rollback_to_latest_savepoint();
    	// BOOST_TEST_REQUIRE(!tokendb.exists_domain("domain"));

     //    tokendb.add_savepoint(get_time());
     //    int pop_re = tokendb.pop_savepoints(get_time());
     //    BOOST_TEST_REQUIRE(pop_re == 0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()