#include <catch/catch.hpp>

#include <jmzk/chain/token_database.hpp>
#include <jmzk/chain/global_property_object.hpp>
#include <jmzk/chain/contracts/jmzk_link_object.hpp>
#include <jmzk/testing/tester.hpp>
#include <jmzk/chain/token_database_cache.hpp>

using namespace jmzk;
using namespace chain;
using namespace contracts;
using namespace testing;
using namespace fc;
using namespace crypto;

extern std::string jmzk_unittests_dir;

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(jmzk::chain::token_type::TYPE, std::nullopt, NAME)

#define EXISTS_TOKEN2(TYPE, DOMAIN, NAME) \
    tokendb.exists_token(jmzk::chain::token_type::TYPE, DOMAIN, NAME)

#define EXISTS_ASSET(ADDR, SYM) \
    tokendb.exists_asset(ADDR, SYM.id())

#define READ_TOKEN(TYPE, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(jmzk::chain::token_type::TYPE, std::nullopt, NAME, str); \
        jmzk::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_TOKEN2(TYPE, DOMAIN, NAME, VALUEREF) \
    { \
        auto str = std::string(); \
        tokendb.read_token(jmzk::chain::token_type::TYPE, DOMAIN, NAME, str); \
        jmzk::chain::extract_db_value(str, VALUEREF); \
    }

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...)      \
    try {                                                                   \
        using vtype = typename decltype(VPTR)::element_type;                \
        VPTR = tokendb_cache.template read_token<vtype>(TYPE, PREFIX, KEY); \
    }                                                                       \
    catch(token_database_exception&) {                                      \
        jmzk_THROW2(EXCEPTION, FORMAT, ##__VA_ARGS__);                       \
    }

#define UPD_DB_TOKEN(TYPE, NAME, VALUE)                                                                         \
    {                                                                                                     \
        tokendb_cache.put_token(jmzk::chain::token_type::TYPE, action_op::update, std::nullopt, NAME, VALUE); \
    }

#define MAKE_PROPERTY(AMOUNT, SYM) \
    property {                     \
        .amount = AMOUNT,          \
        .frozen_amount = 0,        \
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
        jmzk_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM.id()); \
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

#define PUT_DB_ASSET(ADDR, VALUE)                                             \
    while(1) {                                                                \
        if(VALUE.sym.id() == jmzk_SYM_ID) {                                    \
            if constexpr(std::is_same_v<decltype(VALUE), property>) {         \
                auto ps = property_stakes(VALUE);                             \
                auto dv = make_db_value(ps);                                  \
                tokendb.put_asset(ADDR, VALUE.sym.id(), dv.as_string_view()); \
                break;                                                        \
            }                                                                 \
        }                                                                     \
        auto dv = make_db_value(VALUE);                                       \
        tokendb.put_asset(ADDR, VALUE.sym.id(), dv.as_string_view());         \
        break;                                                                \
    }


class contracts_test {
public:
    contracts_test() {
        auto basedir = jmzk_unittests_dir + "/contracts_tests";
        if(!fc::exists(basedir)) {
            fc::create_directories(basedir);
        }

        auto cfg = controller::config();

        cfg.blocks_dir             = basedir + "/blocks";
        cfg.state_dir              = basedir + "/state";
        cfg.db_config.db_path      = basedir + "/tokendb";
        cfg.contracts_console      = true;
        cfg.charge_free_mode       = false;
        cfg.loadtest_mode          = false;
        cfg.max_serialization_time = std::chrono::hours(1);

        cfg.genesis.initial_timestamp = fc::time_point::now();
        cfg.genesis.initial_key       = tester::get_public_key("jmzk");
        auto privkey                  = tester::get_private_key("jmzk");
        my_tester.reset(new tester(cfg));

        my_tester->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

        key_seeds.push_back(N(key));
        key_seeds.push_back("jmzk");
        key_seeds.push_back("jmzk2");
        key_seeds.push_back(N(payer));
        key_seeds.push_back(N(poorer));

        key         = tester::get_public_key(N(key));
        private_key = tester::get_private_key(N(key));
        payer       = address(tester::get_public_key(N(payer)));
        poorer      = address(tester::get_public_key(N(poorer)));

        my_tester->add_money(payer, asset(1'000'000'000'000, jmzk_sym()));

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
    get_sym_id(int seq = 0) {
        auto sym_id = 3 + seq;

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
