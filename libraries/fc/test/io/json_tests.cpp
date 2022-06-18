#define BOOST_TEST_MODULE json test
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <fc/variant_object.hpp>

void
check_variants_equal(const fc::variant& v1, const fc::variant& v2) {
    using namespace fc;

    BOOST_TEST_CHECK(v1.get_type() == v2.get_type());
    switch(v1.get_type()) {
    case variant::int64_type: {
        BOOST_TEST_CHECK(v1.as_int64() == v2.as_int64());
        break;
    }
    case variant::uint64_type: {
        BOOST_TEST_CHECK(v1.as_uint64() == v2.as_uint64());
        break;
    }
    case variant::double_type: {
        BOOST_TEST_CHECK(v1.as_double() == v2.as_double());
        break;
    }
    case variant::bool_type: {
        BOOST_TEST_CHECK(v1.as_bool() == v2.as_bool());
        break;
    }
    case variant::string_type: {
        BOOST_TEST_CHECK(v1.as_string() == v2.as_string());
        break;
    }
    case variant::array_type: {
        auto& a1 = v1.get_array();
        auto& a2 = v2.get_array();
        BOOST_TEST_CHECK(a1.size() == a2.size());
        for(auto i = 0u; i < a1.size(); i++) {
            check_variants_equal(a1[i], a2[i]);
        }
        break;
    }
    case variant::object_type: {
        auto& o1 = v1.get_object();
        auto& o2 = v2.get_object();
        BOOST_TEST_CHECK(o1.size() == o2.size());
        auto it1 = o1.begin();
        auto it2 = o2.begin();
        for(; it1 != o1.end(); it1++, it2++) {
            check_variants_equal(it1->value(), it2->value());
        }
        break;
    }
    default: {
        BOOST_TEST_CHECK(false, "Not supported check");
    }
    } // switch
}

const char* sample_json() {
    auto json = R"(
    {
        "name": "test",
        "issuer": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "issue": {
            "name": "issue",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 1
            }]
        },
        "transfer": {
            "name": "transfer",
            "threshold": 1,
            "authorizers": [{
                "ref": "[G] OWNER",
                "weight": 1
            }]
        },
        "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 1
            }]
        }
    }
    )";
    return json;
}

BOOST_AUTO_TEST_CASE( deseriazlie ) {
    auto json = sample_json();

    fc::variant v1, v2;
    v1 = fc::json::from_string(json, fc::json::legacy_parser);
    v2 = fc::json::from_string(json, fc::json::rapidjson_parser);
    check_variants_equal(v1, v2);
}

BOOST_AUTO_TEST_CASE( deseriazliestr ) {
    auto json = R"("hello")";

    fc::variant v1, v2;
    v1 = fc::json::from_string(json, fc::json::legacy_parser);
    v2 = fc::json::from_string(json, fc::json::rapidjson_parser);
    check_variants_equal(v1, v2);
}

BOOST_AUTO_TEST_CASE( serialize ) {
    auto json = sample_json();
    auto v    = fc::json::from_string(json);

    auto j1 = fc::json::to_string(v, fc::json::stringify_large_ints_and_doubles);
    auto j2 = fc::json::to_string(v, fc::json::rapidjson_generator);

    auto v1 = fc::json::from_string(j1);
    auto v2 = fc::json::from_string(j2);

    check_variants_equal(v1, v2);
}