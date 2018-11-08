#include <sstream>

#include <catch/catch.hpp>

#include <evt/chain/token_database.hpp>
#include <evt/chain/token_database_snapshot.hpp>
#include <evt/chain/contracts/types.hpp>

using namespace evt;
using namespace chain;

extern std::string evt_unittests_dir;

string tokendb_ss;

TEST_CASE("tokendb_save", "[snapshot]") {
    auto db = token_database();
    db.initialize(evt_unittests_dir + "/snapshot_tests");

    auto d = domain_def();
    d.name = "test-domain";

    db.add_domain(d);

    auto it   = issuetoken();
    it.domain = d.name;
    it.owner.emplace_back(address());
    for(int i = 0; i < 10000; i++) {
        it.names.emplace_back(std::to_string(i));
    }
    db.issue_tokens(it);

    auto ss = std::stringstream();
    auto writer = std::make_shared<ostream_snapshot_writer>(ss);

    auto tds = token_database_snapshot();
    tds.add_to_snapshot(writer, db);

    tokendb_ss = ss.str();
}

TEST_CASE("tokendb_load", "[snapshot]") {
    printf("load\n");

    auto ss = std::stringstream(tokendb_ss);
    auto reader = std::make_shared<istream_snapshot_reader>(ss);

    auto tds = token_database_snapshot();
    tds.read_from_snapshot(reader, evt_unittests_dir + "/snapshot_tests");
}