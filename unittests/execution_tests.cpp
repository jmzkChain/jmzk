#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include <vector>

#include <catch/catch.hpp>

#include <fc/io/json.hpp>

#include <evt/chain/contracts/types.hpp>
#include <evt/chain/execution/execution_context.hpp>

using namespace evt::chain;

struct test {
    EVT_ACTION_VER0(test);
};

struct test2 {
    EVT_ACTION_VER1(test, test2);
};

using execution::execution_context;
using execution_context_test = execution_context<
                                   contracts::newdomain,
                                   contracts::updatedomain,
                                   contracts::issuetoken,
                                   contracts::transfer,
                                   test,
                                   test2
                               >;

class execution_tests {
public:
    execution_tests()
        : abi_(contracts::evt_contract_abi(), fc::milliseconds(config::default_abi_serializer_max_time_ms)) {}

public:
    execution_context_test    ctx_;
    contracts::abi_serializer abi_;
};

TEST_CASE_METHOD(execution_tests, "test_index_of", "[execution]") {
    CHECK(N(issuetoken) < N(newdomain));
    CHECK(ctx_.index_of("issuetoken") == 0);
    CHECK(ctx_.index_of("newdomain") == 1);
}

TEST_CASE_METHOD(execution_tests, "test_serializer", "[execution]") {
    const char* test_data = R"=====(
    {
      "name" : "cookie",
      "creator" : "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "issue" : {
        "name" : "issue",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      },
      "transfer": {
        "name": "transfer",
        "threshold": 1,
        "authorizers": [{
            "ref": "[G] .OWNER",
            "weight": 1
          }
        ]
      },
      "manage": {
        "name": "manage",
        "threshold": 1,
        "authorizers": [{
            "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      }
    }
    )=====";

    auto var = fc::json::from_string(test_data);

    auto b1 = ctx_.variant_to_binary(ctx_.index_of(N(newdomain)), var);
    auto b2 = abi_.variant_to_binary("newdomain", var);

    CHECK(b1 == b2);
}

template<uint64_t N>
struct tinvoke {
    template <typename Type>
    static uint64_t invoke(Type type, int& ver) {
        CHECK(type != hana::type_c<contracts::newdomain>);
        CHECK(ver == 0);
        return N;
    }
};

template<>
struct tinvoke<N(newdomain)> {
    template <typename Type>
    static uint64_t invoke(Type type, int& ver) {
        CHECK(type == hana::type_c<contracts::newdomain>);
        CHECK(ver == 0);
        return 0;
    }
};

template<>
struct tinvoke<N(test)> {
    template <typename Type>
    static uint64_t invoke(Type type, int& ver) {
        if(ver == 0) {
            CHECK(type == hana::type_c<test>);
        }
        else if(ver == 1) {
            CHECK(type == hana::type_c<test2>);
        }
        return decltype(type)::type::get_type_name().value;
    }
};

TEST_CASE_METHOD(execution_tests, "test_invoke", "[execution]") {
    auto ind = ctx_.index_of(N(newdomain));
    auto iit = ctx_.index_of(N(issuetoken));
    auto ite = ctx_.index_of(N(test));

    CHECK(ctx_.invoke<uint64_t, tinvoke, int>(ind, 0) == 0);
    CHECK(ctx_.invoke<uint64_t, tinvoke, int>(iit, 0) == N(issuetoken));

    CHECK(ctx_.invoke<uint64_t, tinvoke, int>(ite, 0) == N(test));
    // update version of `test` to 1
    CHECK(ctx_.set_version("test", 1) == 0);
    CHECK(ctx_.invoke<uint64_t, tinvoke, int>(ite, 1) == N(test2));

}