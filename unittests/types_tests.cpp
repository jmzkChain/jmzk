#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

#include <evt/chain/types.hpp>
#include <evt/chain/address.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>

using namespace evt::chain;
using namespace evt::chain::contracts;

BOOST_AUTO_TEST_SUITE(types_tests)

BOOST_AUTO_TEST_CASE(test_address) {
    try {
        auto addr = address();

        BOOST_TEST(addr.is_reserved());
        auto var1 = fc::variant();
        fc::to_variant(addr, var1);
        BOOST_TEST(var1.is_string());
        BOOST_TEST(var1.get_string() == "EVT00000000000000000000000000000000000000000000000000");

        address addr2;
        fc::from_variant(var1, addr2);
        BOOST_TEST(addr2.is_reserved());

        BOOST_TEST(addr == addr2);

        auto keystr = std::string("EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");
        auto pkey = public_key_type(keystr);

        addr.set_public_key(pkey);
        BOOST_TEST(addr.is_public_key());
        auto var2 = fc::variant();
        fc::to_variant(addr, var2);
        BOOST_TEST(var2.is_string());
        BOOST_TEST(var2.get_string() == "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");

        address addr3;
        fc::from_variant(var2, addr3);
        BOOST_TEST(addr3.is_public_key());
        BOOST_TEST(addr3.get_public_key() == pkey);

        BOOST_TEST(addr == addr3);

        addr.set_generated("xxxxxxxxxxxx", "xxxxxxxxxxxxxxxxxxxxx");
        BOOST_TEST(addr.is_generated());
        auto var3 = fc::variant();
        fc::to_variant(addr, var3);

        address addr4;
        fc::from_variant(var3, addr4);
        BOOST_TEST(addr4.is_generated());
        BOOST_TEST(addr4.get_prefix() == "xxxxxxxxxxxx");
        BOOST_TEST(addr4.get_key() == "xxxxxxxxxxxxxxxxxxxxx");

        BOOST_TEST(addr == addr4);
        BOOST_TEST_MESSAGE(addr4.to_string());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()