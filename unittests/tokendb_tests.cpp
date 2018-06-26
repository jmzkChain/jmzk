#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <time.h>
#include <vector>

#include <evt/chain/contracts/types.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/controller.hpp>

#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <fc/variant.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;

extern std::string tokendb_dir;

struct tokendb_test {
    tokendb_test() {
        tokendb.initialize(tokendb_dir);
        ti = 0;
    }
    ~tokendb_test() {}

    int32_t
    get_time() {
        return time(0) + (ti++);
    }

    token_database tokendb;
    int            ti;
};

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

    auto       var  = fc::json::from_string(test_data);
    issuetoken istk = var.as<issuetoken>();
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
        ]
        "metas":[{
      	"key": "key",
      	"value": "value",
      	"creator": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
      }]
    }
    )=====";

    auto      var = fc::json::from_string(test_data);
    token_def tk  = var.as<token_def>();
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

    auto      var = fc::json::from_string(test_data);
    group_def gp  = var.as<group_def>();
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

    auto      var = fc::json::from_string(test_data);
    group_def gp  = var.as<group_def>();
    return gp;
}

BOOST_FIXTURE_TEST_SUITE(tokendb_tests, tokendb_test)

BOOST_AUTO_TEST_CASE(tokendb_adddomain_test) {
    try {
        BOOST_CHECK(true);

        auto dom = add_domain_data();
        BOOST_TEST(!tokendb.exists_domain(dom.name));

        auto re = tokendb.add_domain(dom);
        BOOST_TEST_REQUIRE(re == 0);
        BOOST_TEST(tokendb.exists_domain(dom.name));

        domain_def dom_;
        tokendb.read_domain(dom.name, dom_);
        BOOST_TEST(dom.name == dom_.name);
        BOOST_TEST(dom.create_time.to_iso_string() == dom_.create_time.to_iso_string());

        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)dom_.creator);

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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updatedomain_test) {
    try {
        domain_def dom = update_domain_data();
        BOOST_TEST_REQUIRE(tokendb.exists_domain(dom.name));
        dom.metas[0].key = "key" + boost::lexical_cast<std::string>(time(0));

        auto re          = tokendb.update_domain(dom);
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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_issuetoken_test) {
    try {
        issuetoken istk = issue_tokens_data();
        BOOST_TEST(!tokendb.exists_token(istk.domain, istk.names[0]));
        BOOST_TEST(!tokendb.exists_token(istk.domain, istk.names[1]));

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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updatetoken_test) {
    try {
        token_def tk    = update_token_data();
        tk.metas[0].key = "key" + boost::lexical_cast<std::string>(time(0));

        auto re         = tokendb.update_token(tk);
        BOOST_TEST_REQUIRE(re == 0);

        token_def tk_;
        tokendb.read_token(tk.domain, tk.name, tk_);

        BOOST_TEST("domain" == tk_.domain);
        BOOST_TEST(tk.name == tk_.name);
        BOOST_TEST(tk.owner == tk_.owner);

        BOOST_TEST_REQUIRE(1 == tk_.metas.size());
        BOOST_TEST(tk.metas[0].key == tk_.metas[0].key);
        BOOST_TEST("value" == tk_.metas[0].value);
        BOOST_TEST("EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK" == (std::string)tk_.metas[0].creator);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_addgroup_test) {
    try {
        group_def gp = add_group_data();
        BOOST_TEST(!tokendb.exists_group(gp.name_));

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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_updategroup_test) {
    try {
        group_def gp = update_group_data();
        auto      re = tokendb.update_group(gp);

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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(tokendb_fungible_test) {
    auto tmp_fungible = fungible_def();

    BOOST_TEST(!tokendb.exists_fungible("EVT"));
    BOOST_TEST(!tokendb.exists_fungible(symbol(SY(5,EVT))));
    BOOST_CHECK_THROW(tokendb.read_fungible("EVT", tmp_fungible), tokendb_fungible_not_found);
    BOOST_CHECK_THROW(tokendb.read_fungible(symbol(SY(5,EVT)), tmp_fungible), tokendb_fungible_not_found);

    auto evt_fungible = fungible_def();
    evt_fungible.sym = symbol(SY(5,EVT));
    auto r = tokendb.add_fungible(evt_fungible);
    BOOST_TEST(r == 0);

    BOOST_TEST(tokendb.exists_fungible("EVT"));
    BOOST_TEST(tokendb.exists_fungible(symbol(SY(5,EVT))));
    BOOST_TEST(tokendb.exists_fungible(symbol(SY(4,EVT))));
    
    BOOST_CHECK_NO_THROW(tokendb.read_fungible("EVT", tmp_fungible));
    BOOST_TEST(tmp_fungible.sym == symbol(SY(5,EVT)));
    BOOST_CHECK_NO_THROW(tokendb.read_fungible(symbol(SY(5,EVT)), tmp_fungible));
    BOOST_TEST(tmp_fungible.sym == symbol(SY(5,EVT)));

    auto tmp_asset = asset();
    auto address1 = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    auto address2 = public_key_type(std::string("EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"));
    BOOST_TEST(!tokendb.exists_any_asset(address1));
    BOOST_TEST(!tokendb.exists_asset(address1, symbol(SY(5,EVT))));
    BOOST_CHECK_THROW(tokendb.read_asset(address1, symbol(SY(5,EVT)), tmp_asset), tokendb_asset_not_found);
    BOOST_CHECK_NO_THROW(tokendb.read_asset_no_throw(address1, symbol(SY(5,EVT)), tmp_asset));
    BOOST_CHECK(tmp_asset == asset(0, symbol(SY(5,EVT))));
    
    auto s = 0;
    tokendb.read_all_assets(address1, [&](const auto&) { s++; return true; });
    BOOST_TEST(s == 0);

    auto r1 = tokendb.update_asset(address1, asset(2000, symbol(SY(5,EVT))));
    auto r2 = tokendb.update_asset(address1, asset(1000, symbol(SY(8,ETH))));

    BOOST_TEST(r1 == 0);
    BOOST_TEST(r2 == 0);

    BOOST_TEST(tokendb.exists_any_asset(address1));
    BOOST_TEST(tokendb.exists_asset(address1, symbol(SY(5,EVT))));
    BOOST_TEST(tokendb.exists_asset(address1, symbol(SY(8,ETH))));
    BOOST_TEST(!tokendb.exists_asset(address1, symbol(SY(4,EVT))));
    BOOST_CHECK_NO_THROW(tokendb.read_asset(address1, symbol(SY(5,EVT)), tmp_asset));
    BOOST_CHECK(tmp_asset == asset(2000, symbol(SY(5,EVT))));

    auto s2 = 0;
    tokendb.read_all_assets(address1, [&](const auto& s) { BOOST_TEST_MESSAGE((std::string)s); s2++; return true; });
    BOOST_TEST(s2 == 2);
}

BOOST_AUTO_TEST_CASE(tokendb_checkpoint_test) {
    try {
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

        BOOST_TEST_REQUIRE(tokendb.exists_token(dom.name, "t1"));
        token_def tk_;
        tokendb.read_token(dom.name, "t1", tk_);
        BOOST_TEST_REQUIRE(1 == tk_.metas.size());
        tokendb.rollback_to_latest_savepoint();
        tokendb.read_token(dom.name, "t1", tk_);
        BOOST_TEST(0 == tk_.metas.size());
        tokendb.rollback_to_latest_savepoint();
        BOOST_TEST_REQUIRE(!tokendb.exists_token(dom.name, "t1"));

        BOOST_TEST_REQUIRE(tokendb.exists_domain(dom.name));
        domain_def dom_;
        tokendb.read_domain(dom.name, dom_);
        BOOST_TEST_REQUIRE(1 == dom_.metas.size());
        tokendb.rollback_to_latest_savepoint();
        tokendb.read_domain(dom.name, dom_);
        BOOST_TEST_REQUIRE(0 == dom_.metas.size());
        tokendb.rollback_to_latest_savepoint();
        BOOST_TEST_REQUIRE(!tokendb.exists_domain(dom.name));

        tokendb.add_savepoint(get_time());
        group_def gp = add_group_data();
        gp.name_     = "group-" + boost::lexical_cast<std::string>(time(0));
        tokendb.add_group(gp);
        tokendb.add_savepoint(get_time());

        group_def upgp = update_group_data();
        upgp.name_     = gp.name();
        tokendb.update_group(upgp);

        BOOST_TEST_REQUIRE(tokendb.exists_group(gp.name()));
        group_def gp_;
        tokendb.read_group(gp.name(), gp_);
        auto root = gp_.root();
        BOOST_TEST(5 == root.threshold);
        tokendb.rollback_to_latest_savepoint();
        tokendb.read_group(gp.name(), gp_);
        root = gp_.root();
        BOOST_TEST(6 == root.threshold);
        tokendb.rollback_to_latest_savepoint();
        BOOST_TEST_REQUIRE(!tokendb.exists_group(gp.name()));

        tokendb.add_savepoint(get_time());
        gp       = add_group_data();
        gp.name_ = "group--" + boost::lexical_cast<std::string>(time(0));
        tokendb.add_group(gp);
        tokendb.add_savepoint(get_time());

        upgp       = update_group_data();
        upgp.name_ = gp.name();
        tokendb.update_group(upgp);

        tokendb.add_savepoint(get_time());
        int pop_re = tokendb.pop_savepoints(get_time());
        BOOST_TEST_REQUIRE(pop_re == 0);

        auto address = public_key_type((std::string)"EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX");
        BOOST_TEST(!tokendb.exists_fungible("EVT"));
        BOOST_TEST(!tokendb.exists_any_asset(address));
        BOOST_TEST(!tokendb.exists_asset(address, symbol(SY(5,EVT))));
        
        fungible_def fungible;
        fungible.sym = symbol(SY(5,EVT));
        tokendb.add_fungible(fungible);
        tokendb.update_asset(address, asset(1000,symbol(SY(5,EVT))));

        BOOST_TEST(tokendb.exists_fungible("EVT"));
        BOOST_TEST(tokendb.exists_asset(address, symbol(SY(5,EVT))));

        tokendb.add_savepoint(get_time());
        tokendb.update_asset(address, asset(2000,symbol(SY(5,EVT))));

        tokendb.rollback_to_latest_savepoint();
        asset a;
        tokendb.read_asset(address, symbol(SY(5,EVT)), a);
        BOOST_TEST(a == asset(1000,symbol(SY(5,EVT))));

        tokendb.rollback_to_latest_savepoint();
        BOOST_TEST(!tokendb.exists_fungible("EVT"));
        BOOST_TEST(!tokendb.exists_any_asset(address));
        BOOST_TEST(!tokendb.exists_asset(address, symbol(SY(5,EVT))));

        tokendb.pop_savepoints(0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
