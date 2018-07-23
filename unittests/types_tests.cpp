#include <boost/test/framework.hpp>
#include <boost/test/unit_test.hpp>

#include <evt/chain/types.hpp>
#include <evt/chain/address.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/evt_link.hpp>

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

        addr.set_generated("xxxxxxxxxxxx", "xxxxxxxxxxxxxxxxxxxxx", 1234);
        BOOST_TEST(addr.is_generated());
        auto var3 = fc::variant();
        fc::to_variant(addr, var3);

        address addr4;
        fc::from_variant(var3, addr4);
        BOOST_TEST(addr4.is_generated());
        BOOST_TEST(addr4.get_prefix() == "xxxxxxxxxxxx");
        BOOST_TEST(addr4.get_key() == "xxxxxxxxxxxxxxxxxxxxx");
        BOOST_TEST(addr4.get_nonce() == 1234);

        BOOST_TEST(addr == addr4);
        BOOST_TEST_MESSAGE(addr4.to_string());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(test_link) {
    try {
        auto str = "https://evt.li/4747475658950104272241092099839773980437429267601623920250082837965075865363822-1434796614004741756602070644829168553854103836338494903274751752814999311920619538392151321614659275331665864128126232202027142459066697865188692558466935058141712657741125018335620670444245392083141425797346391551610479926002656405529648417098324800658139566355692210939166021976963801969064062525433132038057122";
        auto link = evt_link::parse_from_evtli(str);

        BOOST_TEST(link.get_segment(41).intv == 8);
        BOOST_TEST(link.get_segment(42).intv == 1532234776);
        BOOST_TEST(link.get_segment(91).intv == 0);
        BOOST_TEST(link.get_segment(91).strv == "testdomain");
        BOOST_TEST(link.get_segment(92).strv == "testtoken");

        auto sigs = link.get_signatures();
        BOOST_TEST(sigs.size() == 1);

        auto s1 = sigs.find(signature_type(std::string("SIG_K1_K1Bg7cBXmxFyR9BtQyKu2yLZn9DMwaTougvF4nHxJGMsqpKi7AcRgu8bJrzCbnPxKDoiCiQX9siRpkznaRcvSn4m1X8w5C"))) != sigs.end();
        BOOST_TEST(s1 == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()