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
        tokendb.initialize("/tmp/tokendb/");
    }
    ~tokendb_test() {}

    token_database tokendb;
};

BOOST_FIXTURE_TEST_SUITE(tokendb_tests, tokendb_test)

BOOST_AUTO_TEST_CASE(tokendb_adddomain_test) {
    try {
        BOOST_CHECK(true);
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

        tokendb.read_domain(dom.name, [&dom](const domain_def& dom_) {
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

        });
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updatedomain_test) {
    try {
        BOOST_CHECK(true);
        const char* test_data = R"=====(
	    {
	     "name" : "domain"
	      	
	    }
	    )=====";
	     // "name" : "domain",
	     //  "issue" : {
	     //    "name" : "issue1",
	     //    "threshold" : 11,
	     //    "authorizers": [{
	     //        "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
	     //        "weight": 1
	     //      }
	     //    ]
	     //  },
	     // "transfer": {
	     //    "name": "transfer",
	     //    "threshold": 1,
	     //    "authorizers": [{
	     //        "ref": "[G] OWNER",
	     //        "weight": 1
	     //      }
	     //    ]
	     //  },
	     //  "manage": {
	     //    "name": "manage",
	     //    "threshold": 1,
	     //    "authorizers": [{
	     //        "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
	     //        "weight": 1
	     //      }
	     //    ]
	     //  }
	    // "metas":[{
	    //   	"key": "key",
	    //   	"value": "value",
	    //   	"creator": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
	    //   }]

        auto       var = fc::json::from_string(test_data);
        db_updatedomain dom = var.as<db_updatedomain>();

        BOOST_TEST_REQUIRE(tokendb.exists_domain(dom.name));
        auto re = tokendb.update_domain(dom);

        BOOST_TEST_REQUIRE(re == 0);

        // tokendb.read_domain(dom.name, [&dom](const domain_def& dom_) {
        //     BOOST_TEST(dom.name == dom_.name);

	       //  BOOST_TEST("issue" == dom_.issue.name);
	       //  BOOST_TEST(1 == dom_.issue.threshold);
	       //  BOOST_TEST_REQUIRE(1 == dom_.issue.authorizers.size());
	       //  BOOST_TEST(dom_.issue.authorizers[0].ref.is_account_ref());
	       //  BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.issue.authorizers[0].ref.get_account());
	       //  BOOST_TEST(1 == dom_.issue.authorizers[0].weight);

	       //  BOOST_TEST("transfer" == dom_.transfer.name);
	       //  BOOST_TEST(1 == dom_.transfer.threshold);
	       //  BOOST_TEST_REQUIRE(1 == dom_.transfer.authorizers.size());
	       //  BOOST_TEST(dom_.transfer.authorizers[0].ref.is_owner_ref());
	       //  BOOST_TEST(1 == dom_.transfer.authorizers[0].weight);

	       //  BOOST_TEST("manage" == dom_.manage.name);
	       //  BOOST_TEST(1 == dom_.manage.threshold);
	       //  BOOST_TEST_REQUIRE(1 == dom_.manage.authorizers.size());
	       //  BOOST_TEST(dom_.manage.authorizers[0].ref.is_account_ref());
	       //  BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.manage.authorizers[0].ref.get_account());
	       //  BOOST_TEST(1 == dom_.manage.authorizers[0].weight);

	       //  BOOST_TEST_REQUIRE(1 == dom_.metas.size());
	       //  BOOST_TEST("key" == dom_.metas[0].key);
	       //  BOOST_TEST("value" == dom_.metas[0].value);
	       //  BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.metas[0].creator);

        // });
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_issuetoken_test) {
    try {
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
        istk.names[0]     = "t1" + boost::lexical_cast<std::string>(time(0));
        istk.names[1]     = "t2" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.issue_tokens(istk);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[0]));
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[1]));

        tokendb.read_token(istk.domain,istk.names[0], [&istk](const token_def& tk_) {
            BOOST_TEST("domain" == tk_.domain);

	        BOOST_TEST(istk.names[0] == tk_.name);
	        BOOST_TEST(istk.owner == tk_.owner);
        });
        tokendb.read_token(istk.domain,istk.names[1], [&istk](const token_def& tk_) {
            BOOST_TEST("domain" == tk_.domain);

	        BOOST_TEST(istk.names[1] == tk_.name);
	        BOOST_TEST(istk.owner == tk_.owner);
        });
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_addgroup_test) {
    try {
        const char* test_data = R"=====(
	    {
			"name": "5jxXg",
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
        gp.name_       = "group" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.add_group(gp);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_group(gp.name_));

        tokendb.read_group(gp.name(), [&gp](const group_def& gp_) {
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
        });
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_addaccount_test) {
    try {
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
        acct.name      = "account" + boost::lexical_cast<std::string>(time(0));

        auto re = tokendb.add_account(acct);

        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_account(acct.name));

        tokendb.read_account(acct.name, [&acct](const account_def& acct_) {
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
        });
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

    	int add_re = tokendb.add_savepoint(time(0));
        BOOST_TEST_REQUIRE(add_re == 0);
    	const char* test_data = R"=====(
	    {
	      "domain": "domain",
	        "names": [
	          "t",
	        ],
	        "owner": [
	          "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
	        ]
	    }
	    )=====";

        auto       var  = fc::json::from_string(test_data);
        issuetoken istk = var.as<issuetoken>();
        istk.names[0]     = "t" + boost::lexical_cast<std::string>(time(0));
        tokendb.issue_tokens(istk);

        tokendb.add_savepoint(time(0)+1);
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[0]));

        int roll_re = tokendb.rollback_to_latest_savepoint();
        BOOST_TEST_REQUIRE(roll_re == 0);
        BOOST_TEST(tokendb.exists_token(istk.domain, istk.names[0]));
        tokendb.rollback_to_latest_savepoint();
         BOOST_TEST(!tokendb.exists_token(istk.domain, istk.names[0]));

        tokendb.add_savepoint(time(0)+2);
        int pop_re = tokendb.pop_savepoints(time(0));
        BOOST_TEST_REQUIRE(pop_re == 0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()