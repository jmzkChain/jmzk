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

string tokendb_ss;

auto get_db_config() = []{
    auto c = token_database::config();
    c.db_path = evt_unittests_dir + "/snapshot_tests";
    return c;
};

TEST_CASE("tokendb_setup", "[snapshot]") {
    auto db = token_database(get_db_config());
    db.open();

    // add savepoint #1
    db.add_savepoint(1);

    auto d = domain_def();
    d.name = "test-domain";
    db.add_domain(d);

    // add savepoint #2
    db.add_savepoint(2);

    auto it   = issuetoken();
    it.domain = d.name;
    it.owner.emplace_back(address());
    for(int i = 0; i < 10; i++) {
        it.names.emplace_back(std::to_string(i));
    }
    db.issue_tokens(it);
}

TEST_CASE("tokendb_save", "[snapshot]") {
    auto db = token_database(get_db_config());
    db.open();

    // should have two savepoints #1 & #2
    REQUIRE(db.savepoints_size() == 2);
    REQUIRE(db.exists_domain("test-domain"));

    // add savepoint #3
    db.add_savepoint(3);

    auto d = domain_def();
    d.name = "test-domain-2";
    db.add_domain(d);

    // add token db to snapshot with savepoints (#1, #2 -> persist sp) (#3 -> runtime sp)
    auto ss = std::stringstream();
    auto writer = std::make_shared<ostream_snapshot_writer>(ss);
    
    token_database_snapshot::add_to_snapshot(writer, db);
    tokendb_ss = ss.str();

    CHECK(db.exists_domain("test-domain-2"));
}

TEST_CASE("tokendb_load", "[snapshot]") {
    auto db = token_database(get_db_config());
    db.open();

    // still have two savepoints #1, #2, #3
    REQUIRE(db.savepoints_size() == 3);

    // add savepoint #4
    db.add_savepoint(4);

    auto d = domain_def();
    d.name = "test-domain-3";
    db.add_domain(d);

    // restore tokendb from snapshot
    auto ss = std::stringstream(tokendb_ss);
    auto reader = std::make_shared<istream_snapshot_reader>(ss);

    token_database_snapshot::read_from_snapshot(reader, db);

    REQUIRE(db.savepoints_size() == 3);

    CHECK(db.exists_domain("test-domain"));
    for(int i = 0; i < 10; i++) {
        CHECK(db.exists_token("test-domain", std::to_string(i)));
    }
    CHECK(db.exists_domain("test-domain-2"));
    CHECK(!db.exists_domain("test-domain-3"));
}