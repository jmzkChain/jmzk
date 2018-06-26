#define BOOST_TEST_MODULE evt unittests
#define BOOST_TEST_DYN_LINK
#include <boost/test/included/unit_test.hpp>
#include <fc/filesystem.hpp>

std::string tokendb_dir = "tmp/evt_tokendb";

struct GlobalFixture {
    void
    setup() {
        if(fc::exists(tokendb_dir)) {
            fc::remove_all(tokendb_dir);
        }
    }

    void
    teardown() {
        fc::remove_all(tokendb_dir);
    }
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);