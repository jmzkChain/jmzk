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

#define ADD_TOKEN(TYPE, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::add, std::nullopt, KEY, dbvalue.as_string_view()); \
    }

#define UPDATE_TOKEN(TYPE, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::update, std::nullopt, KEY, dbvalue.as_string_view()); \
    }

#define PUT_TOKEN(TYPE, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::put, std::nullopt, KEY, dbvalue.as_string_view()); \
    }

#define ADD_TOKEN2(TYPE, DOMAIN, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::add, DOMAIN, KEY, dbvalue.as_string_view()); \
    }

#define UPDATE_TOKEN2(TYPE, DOMAIN, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::update, DOMAIN, KEY, dbvalue.as_string_view()); \
    }

#define PUT_TOKEN2(TYPE, DOMAIN, KEY, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_token(evt::chain::token_type::TYPE, evt::chain::action_op::put, DOMAIN, KEY, dbvalue.as_string_view()); \
    }

#define PUT_ASSET(ADDR, SYMID, DATA) \
    { \
        auto dbvalue = evt::chain::make_db_value(DATA); \
        tokendb.put_asset(ADDR, SYMID, dbvalue.as_string_view()); \
    }

#define ADD_SAVEPOINT \
    tokendb.add_savepoint(tokendb.latest_savepoint_seq()+1)

#define ROLLBACK \
    tokendb.rollback_to_latest_savepoint()
