#include <catch/catch.hpp>

#include <evt/chain/token_database.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>
#include <evt/testing/tester.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

extern std::string evt_unittests_dir;

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, std::nullopt, NAME)

#define EXISTS_TOKEN2(TYPE, DOMAIN, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, DOMAIN, NAME)

#define EXISTS_ASSET(ADDR, SYM) \
    tokendb.exists_asset(ADDR, SYM.id())

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

#define MAKE_PROPERTY(AMOUNT, SYM) \
    property {                     \
        .amount = AMOUNT,          \
        .sym = SYM,                \
        .created_at = 0,           \
        .created_index = 0         \
    }

#define READ_DB_ASSET(ADDR, SYM, VALUEREF)                                                              \
    try {                                                                                               \
        auto str = std::string();                                                                       \
        tokendb.read_asset(ADDR, SYM.id(), str);                                                        \
                                                                                                        \
        extract_db_value(str, VALUEREF);                                                                \
    }                                                                                                   \
    catch(token_database_exception&) {                                                                  \
        EVT_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM.id()); \
    }

#define READ_DB_ASSET_NO_THROW(ADDR, SYM, VALUEREF)                         \
    {                                                                       \
        auto str = std::string();                                           \
        if(!tokendb.read_asset(ADDR, SYM.id(), str, true /* no throw */)) { \
            VALUEREF = MAKE_PROPERTY(0, SYM);                               \
        }                                                                   \
        else {                                                              \
            extract_db_value(str, VALUEREF);                                \
        }                                                                   \
    }

class contracts_test {
public:
    contracts_test() {
        auto basedir = evt_unittests_dir + "/contracts_tests";
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

        my_tester->add_money(payer, asset(1'000'000'000'000, evt_sym()));

        ti = 0;
    }

    ~contracts_test() {
        my_tester->close();
    }

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

    symbol
    get_sym() {
        return symbol(5, get_sym_id());
    }

    int32_t
    get_time() {
        return time(0) + (++ti);
    }

protected:
    public_key_type         key;
    private_key_type        private_key;
    address                 payer;
    address                 poorer;
    std::vector<name>       key_seeds;
    std::unique_ptr<tester> my_tester;
    int                     ti;
    symbol_id_type          sym_id;
};
