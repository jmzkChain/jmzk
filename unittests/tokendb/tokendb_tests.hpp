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
        cfg.blocks_dir            = basedir + "/blocks";
        cfg.state_dir             = basedir + "/state";
        cfg.db_config.db_path     = basedir + "/tokendb";
        cfg.contracts_console     = true;
        cfg.charge_free_mode      = false;
        cfg.loadtest_mode         = false;

        cfg.genesis.initial_timestamp = fc::time_point::now();
        cfg.genesis.initial_key       = tester::get_public_key("evt");
        auto privkey                  = tester::get_private_key("evt");
        my_tester.reset(new tester(cfg));

        my_tester->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        key = tester::get_public_key(N(key));
    }
    ~tokendb_test() {}

protected:
    public_key_type           key;
    std::unique_ptr<tester>   my_tester;
};

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, std::nullopt, NAME)

#define EXISTS_TOKEN2(TYPE, DOMAIN, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, DOMAIN, NAME)

#define EXISTS_ASSET(ADDR, SYMID) \
    tokendb.exists_asset(ADDR, SYMID)

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

#define ADD_SAVEPOINT() \
    tokendb.add_savepoint(tokendb.latest_savepoint_seq()+1)

#define ROLLBACK() \
    tokendb.rollback_to_latest_savepoint()

extern const char* domain_data;
extern const char* token_data;
extern const char* fungible_data;

