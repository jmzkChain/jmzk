#include <boost/test/unit_test.hpp>

#include "evt/chain/name.hpp"

BOOST_AUTO_TEST_SUITE(name_test)

BOOST_AUTO_TEST_CASE(default_to_string)
{
    evt::chain::name name;
    std::string result = name.to_string();
    BOOST_CHECK_EQUAL("", result);

    evt::chain::name128 name128;
    std::string result2 = name128.to_string();
    BOOST_CHECK_EQUAL("", result2);
}

BOOST_AUTO_TEST_CASE(to_string)
{
    evt::chain::name128 t1(".123zxy-ABC");
    auto r1 = t1.to_string();
    BOOST_CHECK_EQUAL(".123zxy-ABC", r1);
    auto n1 = N128(.123zxy-ABC);
    BOOST_CHECK_EQUAL(true, n1 == t1.value);
}

BOOST_AUTO_TEST_SUITE_END()


