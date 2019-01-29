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
#include <evt/testing/tester.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

extern std::string evt_unittests_dir;

class tokendb_test {
public:
    //tokendb_test() : tokendb(evt_unittests_dir + "/tokendb_tests") {
    tokendb_test() {
        auto basedir = evt_unittests_dir + "/tokendb_tests";
        if(!fc::exists(basedir)) {
            fc::create_directories(basedir);
        }

        auto cfg = controller::config();
        cfg.blocks_dir            = basedir + "blocks";
        cfg.state_dir             = basedir + "state";
        cfg.db_config.db_path     = basedir + "tokendb";
        cfg.contracts_console     = true;
        cfg.charge_free_mode      = false;
        cfg.loadtest_mode         = false;

        cfg.genesis.initial_timestamp = fc::time_point::now();
        cfg.genesis.initial_key       = tester::get_public_key("evt");
        auto privkey                  = tester::get_private_key("evt");
        my_tester.reset(new tester(cfg));

        my_tester->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        key_seeds.push_back(N(key));
        key_seeds.push_back("evt");
        key_seeds.push_back("evt2");
        key_seeds.push_back(N(payer));
        key_seeds.push_back(N(poorer));

        key         = tester::get_public_key(N(key));
        private_key = tester::get_private_key(N(key));
        payer       = address(tester::get_public_key(N(payer)));
        poorer      = address(tester::get_public_key(N(poorer)));

        my_tester->add_money(payer, asset(1'000'000'000'000, symbol(5, EVT_SYM_ID)));

        ti = 0;
    }
    ~tokendb_test() {}

protected:
    std::string
    get_domain_name(int seq = 0) {
        static auto base_time = time(0);

        auto name = std::string("domain");
        name.append(std::to_string(base_time + seq));
        return name;
    }

    const char*
    get_group_name() {
        static std::string group_name = "group" + boost::lexical_cast<std::string>(time(0));
        return group_name.c_str();
    }

    const char*
    get_suspend_name() {
        static std::string suspend_name = "suspend" + boost::lexical_cast<std::string>(time(0));
        return suspend_name.c_str();
    }

    const char*
    get_symbol_name() {
        static std::string symbol_name;
        if(symbol_name.empty()) {
            srand((unsigned)time(0));
            for(int i = 0; i < 5; i++)
                symbol_name += rand() % 26 + 'A';
        }
        return symbol_name.c_str();
    }
    
    symbol_id_type
    get_sym_id() {
        auto sym_id = 3;

        return sym_id;
    }

protected:
    public_key_type           key;
    private_key_type          private_key;
    address                   payer;
    address                   poorer;
    std::vector<account_name> key_seeds;
    std::unique_ptr<tester>   my_tester;
    int                       ti;
    symbol_id_type            sym_id;
};

const char* newdomain_data = R"=====(
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

const char* upddomain_data = R"=====(
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

const char* issuetoken_data = R"=====(
    {
        "domain": "domain",
        "names": [
            "t1",
            "t2",
            "t3",
            "t4"
        ],
        "owner": [
            "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK"
        ]
    }
    )=====";

const char* updtoken_data = R"=====(
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

const char* newgroup_data = R"=====(
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

const char* updgroup_data = R"=====(
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

const char* newfungible_data = R"=====(
    {
      "name": "EVT",
      "sym_name": "EVT",
      "sym": "5,S#3",
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
const char* newsuspend_data = R"=======(
    {
        "name": "testsuspend",
        "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
        "trx": {
            "expiration": "2021-07-04T05:14:12",
            "ref_block_num": "3432",
            "ref_block_prefix": "291678901",
            "actions": [
            ],
            "transaction_extensions": []
        }
    }
    )=======";

const char* updsuspend_data = R"=======(
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

const char* newlock_data = R"=======(
    {
        "name": "nftlock",
        "proposer": "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
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
                    "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF",
                    "EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"
                ]
            }
        },
        "succeed": [
        ],
        "failed": [
            "EVT7rbe5ZqAEtwQT6Tw39R29vojFqrCQasK3nT5s2pEzXh1BABXHF"
        ]
    }
    )=======";

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, std::nullopt, NAME)

#define EXISTS_TOKEN2(TYPE, DOMAIN, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, DOMAIN, NAME)

#define EXISTS_ASSET(ADDR, SYM) \
    tokendb.exists_asset(ADDR, SYM)

#define READ_TOKEN(TYPE, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(evt::chain::token_type::TYPE, std::nullopt, NAME, str); \
        evt::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_TOKEN2(TYPE, DOMAIN, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(evt::chain::token_type::TYPE, DOMAIN, NAME, str); \
        evt::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_ASSET(ADDR, SYM, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_asset(ADDR, SYM, str); \
        evt::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                        \
    {                                                                   \
        auto str = std::string();                                       \
        if(!tokendb.read_asset(ADDR, SYM, str, true /* no throw */)) {  \
            VALUEREF = asset(0, SYM);                                   \
        }                                                               \
        else {                                                          \
            extract_db_value(str, VALUEREF);                            \
        }                                                               \
    }

#define PUSH_ACTION(TYPE, DOMAIN, KEY) \
    my_tester->push_action(N(TYPE), name128(DOMAIN), N128(KEY), var.get_object(), key_seeds, payer)

TEST_CASE_METHOD(tokendb_test, "tokendb_newdomain_test", "[tokendb]") {
    auto  var     = fc::json::from_string(newdomain_data);
    auto  dom  = var.as<newdomain>();
    auto& tokendb = my_tester->control->token_db();
    CHECK(!EXISTS_TOKEN(domain, get_domain_name()));
    dom.creator = key;
    dom.name = get_domain_name();
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    to_variant(dom, var);
    PUSH_ACTION(newdomain, get_domain_name(), .create);
    
    CHECK(EXISTS_TOKEN(domain, get_domain_name()));

    domain_def dom_;
    READ_TOKEN(domain, dom.name, dom_);
    CHECK(dom.name == dom_.name);
    CHECK((std::string)key == (std::string)dom_.creator);

    CHECK("issue" == dom_.issue.name);
    CHECK(1 == dom_.issue.threshold);
    REQUIRE(1 == dom_.issue.authorizers.size());
    CHECK(dom_.issue.authorizers[0].ref.is_account_ref());
    CHECK((std::string)key == (std::string)dom_.issue.authorizers[0].ref.get_account());
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
    CHECK((std::string)key == (std::string)dom_.manage.authorizers[0].ref.get_account());
    CHECK(1 == dom_.manage.authorizers[0].weight);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updatedomain_test", "[tokendb]") {
    auto  var     = fc::json::from_string(upddomain_data);
    auto  dom   = var.as<updatedomain>();
    auto& tokendb = my_tester->control->token_db();

    dom.name = get_domain_name();
    dom.issue->authorizers[0].ref.set_account(key);
    dom.manage->authorizers[0].ref.set_account(key);
    to_variant(dom, var);

    PUSH_ACTION(updatedomain, get_domain_name(), .update);

    domain_def dom_;
    READ_TOKEN(domain, get_domain_name(), dom_);

    CHECK(dom.name == dom_.name);

    CHECK("issue" == dom_.issue.name);
    CHECK(1 == dom_.issue.threshold);
    REQUIRE(1 == dom_.issue.authorizers.size());
    CHECK(dom_.issue.authorizers[0].ref.is_account_ref());
    CHECK(std::string(key) == (std::string)dom_.issue.authorizers[0].ref.get_account());
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
    CHECK(std::string(key) == (std::string)dom_.manage.authorizers[0].ref.get_account());
    CHECK(1 == dom_.manage.authorizers[0].weight);
    my_tester->produce_blocks();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_issuetoken_test", "[tokendb]") {
    auto  var     = fc::json::from_string(issuetoken_data);
    auto  istk    = var.as<issuetoken>();
    auto& tokendb = my_tester->control->token_db();

    istk.domain = get_domain_name();
    istk.owner[0] = key;
    to_variant(istk, var);
    
    CHECK(!EXISTS_TOKEN2(token, istk.domain, istk.names[0]));
    CHECK(!EXISTS_TOKEN2(token, istk.domain, istk.names[1]));

    PUSH_ACTION(issuetoken, istk.domain, .issue);

    CHECK(EXISTS_TOKEN2(token, istk.domain, istk.names[0]));
    CHECK(EXISTS_TOKEN2(token, istk.domain, istk.names[1]));

    token_def tk1_;
    token_def tk2_;
    READ_TOKEN2(token, istk.domain, istk.names[0], tk1_);

    CHECK(get_domain_name() == tk1_.domain);
    CHECK(istk.names[0] == tk1_.name);
    CHECK(istk.owner == tk1_.owner);

    READ_TOKEN2(token, istk.domain, istk.names[1], tk2_);

    CHECK(get_domain_name() == tk2_.domain);
    CHECK(istk.names[1] == tk2_.name);
    CHECK(istk.owner == tk2_.owner);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_fungible_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    CHECK(!EXISTS_TOKEN(fungible, 3));

    auto var = fc::json::from_string(newfungible_data);
    auto evt_fungible = var.as<newfungible>();
    evt_fungible.creator = key;
    evt_fungible.issue.authorizers[0].ref.set_account(key);
    evt_fungible.manage.authorizers[0].ref.set_account(key);
    to_variant(evt_fungible, var);
    PUSH_ACTION(newfungible, ".fungible", 3);

    CHECK(EXISTS_TOKEN(fungible, EVT_SYM_ID));

    
    auto address1 = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    CHECK(!EXISTS_ASSET(address1, 3));
    
    const char* issuefungible_data = R"=====(
    {
      "address": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
      "number" : "1.00000 S#3",
      "memo": "tokendb_test"
    }
    )=====";
    var = fc::json::from_string(issuefungible_data);
    auto isfg = var.as<issuefungible>();
    PUSH_ACTION(issuefungible, ".fungible", 3);

    CHECK(EXISTS_ASSET(address1, 3));
    auto tmp_asset = asset();
    READ_ASSET(address1, 3, tmp_asset);
    CHECK(tmp_asset == asset(100000, symbol(5,3)));

    my_tester->produce_blocks();
}

#define ADD_SAVEPOINT \
    tokendb.add_savepoint(tokendb.latest_savepoint_seq()+1);
#define ROLLBACK \
    tokendb.rollback_to_latest_savepoint();

TEST_CASE_METHOD(tokendb_test, "tokendb_savepoint_test0", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    ADD_SAVEPOINT
    ROLLBACK
    my_tester->produce_block();
}


TEST_CASE_METHOD(tokendb_test, "tokendb_savepoint_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    
    my_tester->produce_block();
    ADD_SAVEPOINT
    
    auto dom_name = get_domain_name(tokendb.latest_savepoint_seq());
  
    auto var = fc::json::from_string(newdomain_data);
    auto dom = var.as<newdomain>();
    CHECK(!EXISTS_TOKEN(domain, dom_name));
    dom.creator = key;
    dom.name = dom_name;
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    to_variant(dom, var);
    PUSH_ACTION(newdomain, dom_name, .create);
    CHECK(EXISTS_TOKEN(domain, dom_name));

    ADD_SAVEPOINT

    var = fc::json::from_string(issuetoken_data);
    auto istk = var.as<issuetoken>();

    istk.domain = dom_name;
    istk.owner[0] = key;
    to_variant(istk, var);
    CHECK(!EXISTS_TOKEN2(token, istk.domain, istk.names[0]));
    CHECK(!EXISTS_TOKEN2(token, istk.domain, istk.names[1]));
    PUSH_ACTION(issuetoken, dom_name, .issue);
    CHECK(EXISTS_TOKEN2(token, istk.domain, istk.names[0]));
    CHECK(EXISTS_TOKEN2(token, istk.domain, istk.names[1]));
    
    ROLLBACK

    CHECK(!EXISTS_TOKEN2(token, dom_name, "t1"));
    CHECK(!EXISTS_TOKEN2(token, dom_name, "t2"));

    ROLLBACK
        
    CHECK(!EXISTS_TOKEN(domain, dom_name));   
}


TEST_CASE_METHOD(tokendb_test, "tokendb_newsuspend_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();

    auto var = fc::json::from_string(newsuspend_data);
    auto nsus = var.as<newsuspend>();
    nsus.name = get_suspend_name();
    nsus.trx.payer = tester::get_public_key(N(payer));
    nsus.proposer = key;
    
    auto newdomain_var = fc::json::from_string(newdomain_data);
    auto newdom = newdomain_var.as<newdomain>();
    newdom.creator = tester::get_public_key(N(key));
    newdom.name = get_domain_name();
    newdom.issue.authorizers[0].ref.set_account(key);
    newdom.manage.authorizers[0].ref.set_account(key);
    to_variant(newdom, newdomain_var);
    
    nsus.trx.set_reference_block(my_tester->control->fork_db_head_block_id());
    nsus.trx.actions.push_back(my_tester->get_action(N(newdomain), get_domain_name(), N128(.create), newdomain_var.get_object()));
    to_variant(nsus, var);
    
    my_tester->push_action(N(newsuspend), N128(.suspend), get_suspend_name(), var.get_object(), key_seeds, payer);

    CHECK(EXISTS_TOKEN(suspend, nsus.name));

    auto _sus = suspend_def();
    READ_TOKEN(suspend, nsus.name, _sus);

    CHECK(suspend_status::proposed == _sus.status);
    CHECK(nsus.name == _sus.name);
    CHECK((std::string)key == (std::string)_sus.proposer);
    CHECK("2021-07-04T05:14:12" == _sus.trx.expiration.to_iso_string());
    CHECK(_sus.trx.actions.size() == 1);
    CHECK("newdomain" == _sus.trx.actions[0].name);
    CHECK(get_domain_name() == _sus.trx.actions[0].domain);
    CHECK(".create" == _sus.trx.actions[0].key);
}

TEST_CASE_METHOD(tokendb_test, "tokendb_new_lock_test", "[tokendb]") {
    auto var = fc::json::from_string(newlock_data);
    auto nl = var.as<newlock>();
    auto& tokendb = my_tester->control->token_db();
    

    auto now       = fc::time_point::now();
    nl.unlock_time = now + fc::days(10);
    nl.deadline    = now + fc::days(20);

    CHECK(nl.assets[0].type() == asset_type::tokens);
    nl.assets[0].get<locknft_def>().domain = get_domain_name();
    nl.proposer = tester::get_public_key(N(key));
    nl.condition.get<lock_condkeys>().cond_keys = {tester::get_public_key(N(key))};
    nl.succeed = {public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))};
    to_variant(nl, var);

    CHECK(!EXISTS_TOKEN(lock, nl.name));
    PUSH_ACTION(newlock, ".lock", nftlock);
    CHECK(EXISTS_TOKEN(lock, nl.name));

    lock_def lock_;
    READ_TOKEN(lock, nl.name, lock_);
    CHECK(lock_.status == lock_status::proposed);

    token_def tk;
    READ_TOKEN2(token, get_domain_name(), "t3", tk);
    CHECK(tk.owner.size() == 1);
    CHECK(tk.owner[0] == address(N(.lock), N128(nlact.name), 0));

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_updateprodvote_test", "[tokendb]") {
    const char* test_data = R"======(
        {
            "producer": "evt",
            "key": "key",
            "value": 0
        }
        )======";
    auto var = fc::json::from_string(test_data);
    auto pv = var.as<prodvote>();
    auto& tokendb = my_tester->control->token_db();

    auto vote_sum = flat_map<public_key_type, int64_t>();
    
    pv.key = N128(network-charge-factor);
    pv.value = 1;
    to_variant(pv, var);
    
    PUSH_ACTION(prodvote, ".prodvote", network-charge-factor);
    
    READ_TOKEN(prodvote, pv.key, vote_sum);
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 1);
    vote_sum.clear();

    my_tester->produce_blocks();
}


TEST_CASE_METHOD(tokendb_test, "tokendb_prodvote_presist_test", "[tokendb]") {
    const char* test_data = R"======(
        {
            "producer": "evt",
            "key": "key",
            "value": 0
        }
        )======";
    auto var = fc::json::from_string(test_data);
    auto pv = var.as<prodvote>();
    auto& tokendb = my_tester->control->token_db();

    
    auto vote_sum = flat_map<public_key_type, int64_t>();
    
    pv.key = N128(network-charge-factor);
    pv.value = 1;
    to_variant(pv, var);
    
    PUSH_ACTION(prodvote, ".prodvote", network-charge-factor);
    
    READ_TOKEN(prodvote, pv.key, vote_sum);
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 1);
    vote_sum.clear();

    my_tester->produce_block();
    ADD_SAVEPOINT

    pv.value = 2;
    to_variant(pv, var);
    
    PUSH_ACTION(prodvote, ".prodvote", network-charge-factor);
    
    READ_TOKEN(prodvote, pv.key, vote_sum);
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 2);
    vote_sum.clear();
    
    ROLLBACK
    
    READ_TOKEN(prodvote, pv.key, vote_sum);
    CHECK(vote_sum[tester::get_public_key(pv.producer)] == 1);
    vote_sum.clear();
}

TEST_CASE_METHOD(tokendb_test, "tokendb_squash", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    my_tester->produce_block();
    auto n = tokendb.savepoints_size();
    ADD_SAVEPOINT
    
    auto var = fc::json::from_string(newdomain_data);
    auto dom = var.as<newdomain>();
    dom.name = "squash-test";
    dom.creator = key;
    dom.issue.authorizers[0].ref.set_account(key);
    dom.manage.authorizers[0].ref.set_account(key);
    to_variant(dom, var);
    PUSH_ACTION(newdomain, "squash-test", .create);
    ADD_SAVEPOINT
    
    var = fc::json::from_string(issuetoken_data);
    auto istk = var.as<issuetoken>();
    istk.domain = "squash-test";
    istk.owner[0] = key;
    istk.names.clear();
    istk.names.push_back("squash-t1");
    to_variant(istk, var);
    PUSH_ACTION(issuetoken, "squash-test", .issue);
    ADD_SAVEPOINT
        
    token_def _tk;
    READ_TOKEN2(token, "squash-test", "squash-t1", _tk);
    CHECK(istk.names[0] == _tk.name);
    
    auto num = tokendb.savepoints_size();
    
    ADD_SAVEPOINT
    ADD_SAVEPOINT
    tokendb.squash();
    tokendb.squash();
    
    CHECK(tokendb.savepoints_size() == num);
    
    READ_TOKEN2(token, "squash-test", "squash-t1", _tk);
    CHECK(istk.names[0] == _tk.name);
    CHECK(EXISTS_TOKEN(domain, "squash-test"));
    
    tokendb.squash();
    tokendb.squash();
    tokendb.squash();
    
    CHECK(tokendb.savepoints_size() == n);
}
