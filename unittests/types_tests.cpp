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
        auto str = "https://evt.li/96290245899832575363358600206699034023219293786639516903333848194648461492312827969459254511772333773572552753-4924449932390115536109284013436761115620027267817904582761772787045934572630057696227054209593654627789461858231016375336633065348932608711493830420085667865153878257584839904988004515543249161371225214795417479187650453574197042037221726808722136379975949922079410571043119555269228906128753817406507609182353776886290134968641230400098287104944886751662202114827317956174884362996081031390012002840565446148219681590610088135140122753795706414345027031152493130533252";
        auto link = evt_link::parse_from_evtli(str);

        BOOST_TEST(link.get_segment(41).intv == 11);
        BOOST_TEST(link.get_segment(42).intv == 1532345911);
        BOOST_TEST(link.get_segment(91).intv == 0);
        BOOST_TEST(link.get_segment(91).strv == "nd1532345909735");
        BOOST_TEST(link.get_segment(92).strv == "tk3064691820.4081");

        auto& sigs = link.get_signatures();
        BOOST_TEST(sigs.size() == 3);

        auto s1 = sigs.find(signature_type(std::string("SIG_K1_K17SYsTW138PJGEFBpM6jGFmULX9QAGXszFLEGNGe15CN7fhn6QtHP62ZWKTLJLuU2t8SqRS7RuyEXWydM2JsVCs2eC8pw"))) != sigs.end();
        auto s2 = sigs.find(signature_type(std::string("SIG_K1_JzwzDCWSELtt7H1c87ZckRRT5HE92Z4WREoRaUv3VJwBiCj2eFhm4aWjnvbYwvJgWW7aK8r2K5Dvj5pixDR53FHQmbMmAy"))) != sigs.end();
        auto s3 = sigs.find(signature_type(std::string("SIG_K1_K5wPZivo2314VHCDX7uFgFtQtEPiv3ANjdtpYyG8ReH2A7vxkY3wFjMKbgyher1vY9u7Pjit9y1qAsHjDERmmf7hJ6EDq7"))) != sigs.end();
        BOOST_TEST(s1 == true);
        BOOST_TEST(s2 == true);
        BOOST_TEST(s3 == true);

        auto pkeys = link.restore_keys();
        BOOST_TEST(pkeys.size() == 3);

        auto p1 = pkeys.find(public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))) != pkeys.end();
        auto p2 = pkeys.find(public_key_type(std::string("EVT6MYSkiBHNDLxE6JfTmSA1FxwZCgBnBYvCo7snSQEQ2ySBtpC6s"))) != pkeys.end();
        auto p3 = pkeys.find(public_key_type(std::string("EVT7bUYEdpHiKcKT9Yi794MiwKzx5tGY3cHSh4DoCrL4B2LRjRgnt"))) != pkeys.end();
        BOOST_TEST(p1 == true);
        BOOST_TEST(p2 == true);
        BOOST_TEST(p3 == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()