#include <sstream>

#include <catch/catch.hpp>
#include <fc/filesystem.hpp>

#include <evt/chain/token_database.hpp>
#include <evt/chain/token_database_snapshot.hpp>
#include <evt/chain/contracts/types.hpp>

using namespace evt;
using namespace chain;
using namespace contracts;

extern std::string evt_unittests_dir;

string token_db_snapshot_;

#define EXISTS_TOKEN(TYPE, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, std::nullopt, NAME)

#define EXISTS_TOKEN2(TYPE, DOMAIN, NAME) \
    tokendb.exists_token(evt::chain::token_type::TYPE, DOMAIN, NAME)

#define EXISTS_ASSET(ADDR, SYM_ID) \
    tokendb.exists_asset(ADDR, SYM_ID)

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

#define READ_DB_ASSET(ADDR, SYM_ID, VALUEREF)                                                           \
    try {                                                                                               \
        auto str = std::string();                                                                       \
        tokendb.read_asset(ADDR, SYM_ID, str);                                                          \
                                                                                                        \
        extract_db_value(str, VALUEREF);                                                                \
    }                                                                                                   \
    catch(token_database_exception&) {                                                                  \
        EVT_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM_ID);   \
    }

#define PUT_DB_TOKEN(TYPE, PREFIX, KEY, VALUE)                                                             \
    {                                                                                                      \
        auto dv = make_db_value(VALUE);                                                                    \
        tokendb.put_token(evt::chain::token_type::TYPE, action_op::put, PREFIX, KEY, dv.as_string_view()); \
    }

auto get_db_config = []{
    auto c       = token_database::config();
    auto basedir = evt_unittests_dir + "/tokendb_tests";
    c.db_path    = basedir + "/tokendb";
    return c;
};

TEST_CASE("snapshot_pretest", "[snapshot]") {
    auto tokendb = token_database(get_db_config());
    tokendb.open();

    CHECK(EXISTS_TOKEN(domain, "dm-tkdb-test"));
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-1"));
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-2"));

    auto addr = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    CHECK(EXISTS_ASSET(addr, 3));
}

TEST_CASE("snapshot_save_test", "[snapshot]") {
    auto tokendb = token_database(get_db_config());
    tokendb.open();

    auto ss = std::stringstream();
    auto writer = std::make_shared<ostream_snapshot_writer>(ss);
    
    tokendb.add_savepoint(tokendb.latest_savepoint_seq() + 1);

    auto d = domain_def();
    d.name = "snapshot-domain";

    PUT_DB_TOKEN(domain, std::nullopt, d.name, d);

    token_database_snapshot::add_to_snapshot(writer, tokendb);
    token_db_snapshot_ = ss.str();
}

TEST_CASE("snapshot_load_test", "[snapshot]") {
    auto tokendb = token_database(get_db_config());
    tokendb.open();

    REQUIRE(tokendb.savepoints_size() > 0);

    // restore tokendb from snapshot
    auto ss = std::stringstream(token_db_snapshot_);
    auto reader = std::make_shared<istream_snapshot_reader>(ss);

    token_database_snapshot::read_from_snapshot(reader, tokendb);

    REQUIRE(tokendb.savepoints_size() == 0);
    CHECK(EXISTS_TOKEN(domain, "dm-tkdb-test"));
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-1"));
    CHECK(EXISTS_TOKEN2(token, "dm-tkdb-test", "basic-2"));

    auto addr = public_key_type(std::string("EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"));
    CHECK(EXISTS_ASSET(addr, 3));
    CHECK(EXISTS_TOKEN(domain, "snapshot-domain"));
}
