#include <sstream>

#include <catch/catch.hpp>
#include <fc/filesystem.hpp>

#include <evt/chain/token_database.hpp>
#include <evt/chain/token_database_snapshot.hpp>
#include <evt/chain/contracts/types.hpp>

using namespace evt;
using namespace chain;

extern std::string evt_unittests_dir;

string tokendb_ss;

TEST_CASE("tokendb_setup", "[snapshot]") {
    auto db = token_database(evt_unittests_dir + "/snapshot_tests");
    db.open();

    auto d = domain_def();
    d.name = "test-domain";

    db.add_domain(d);

    auto it   = issuetoken();
    it.domain = d.name;
    it.owner.emplace_back(address());
    for(int i = 0; i < 10; i++) {
        it.names.emplace_back(std::to_string(i));
    }
    db.issue_tokens(it);

}

TEST_CASE("tokendb_save", "[snapshot]") {
    auto db = token_database(evt_unittests_dir + "/snapshot_tests");
    db.open();

    REQUIRE(db.exists_domain("test-domain"));

    auto ss = std::stringstream();
    auto writer = std::make_shared<ostream_snapshot_writer>(ss);

    token_database_snapshot::add_to_snapshot(writer, db);

    tokendb_ss = ss.str();
}

TEST_CASE("tokendb_load", "[snapshot]") {
    auto ss = std::stringstream(tokendb_ss);
    auto reader = std::make_shared<istream_snapshot_reader>(ss);

    auto db_folder = evt_unittests_dir + "/snapshot_tests";
    fc::remove_all(db_folder);

    auto db = token_database(db_folder);
    db.open();

    token_database_snapshot::read_from_snapshot(reader, db);

    CHECK(db.exists_domain("test-domain"));
    for(int i = 0; i < 10; i++) {
        CHECK(db.exists_token("test-domain", std::to_string(i)));
    }
}