#define BOOST_TEST_MODULE evt unittests
#define BOOST_TEST_DYN_LINK
#include <boost/test/included/unit_test.hpp>
#include <fc/filesystem.hpp>

struct GlobalFixture {
    void
    setup() {
        auto dir = "/tmp/tokendb";
        fc::remove_all(dir);
    }

    void teardown() {}
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);