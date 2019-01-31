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

#define ADD_SAVEPOINT \
    tokendb.add_savepoint(tokendb.latest_savepoint_seq()+1)

#define ROLLBACK \
    tokendb.rollback_to_latest_savepoint()
