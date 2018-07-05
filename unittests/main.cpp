#define BOOST_TEST_MODULE evt unittests
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_TOOLS_DEBUGGABLE

#include <boost/test/unit_test.hpp>
#include <fc/filesystem.hpp>

std::string evt_unittests_dir = "tmp/evt_unittests";

struct GlobalFixture {
    void
    setup() {
        if(fc::exists(evt_unittests_dir)) {
            fc::remove_all(evt_unittests_dir);
        }
    }

    void
    teardown() {
        fc::remove_all(evt_unittests_dir);
    }
};

BOOST_TEST_GLOBAL_FIXTURE(GlobalFixture);
