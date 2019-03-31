#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include <vector>

#include <catch/catch.hpp>

#include <fc/io/json.hpp>

#include <evt/chain/execution_context_impl.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/evt_contract_abi.hpp>

using namespace evt::chain;

struct test {
    EVT_ACTION_VER1(test);
};

struct test2 {
    EVT_ACTION_VER2(test, test2);
};

using execution_context_test = execution_context_impl<
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
        : ctx_() {}

public:
    execution_context_test ctx_;
};

TEST_CASE_METHOD(execution_tests, "test_index_of", "[execution]") {
    CHECK(N(issuetoken) < N(newdomain));
    CHECK(ctx_.index_of("issuetoken") == 0);
    CHECK(ctx_.index_of("newdomain") == 1);
    CHECK(ctx_.index_of<contracts::issuetoken>() == 0);
    CHECK(ctx_.index_of<contracts::newdomain>() == 1);
}

template<uint64_t N>
struct tinvoke {
    template <typename Type>
    static std::string invoke(int ver) {
        CHECK(hana::type_c<Type> != hana::type_c<contracts::newdomain>);
        CHECK(ver == 1);
        return Type::get_type_name();
    }
};

template<>
struct tinvoke<N(newdomain)> {
    template <typename Type>
    static std::string invoke(int ver) {
        CHECK(hana::type_c<Type> == hana::type_c<contracts::newdomain>);
        CHECK(ver == 1);
        return Type::get_type_name();
    }
};

template<>
struct tinvoke<N(test)> {
    template <typename Type>
    static std::string invoke(int ver) {
        CHECK(((ver == 1) || (ver == 2)));
        if(ver == 1) {
            CHECK(hana::type_c<Type> == hana::type_c<test>);
        }
        else if(ver == 2) {
            CHECK(hana::type_c<Type> == hana::type_c<test2>);
        }
        return Type::get_type_name();
    }
};

TEST_CASE_METHOD(execution_tests, "test_invoke", "[execution]") {
    auto ind = ctx_.index_of(N(newdomain));
    auto iit = ctx_.index_of(N(issuetoken));
    auto ite = ctx_.index_of(N(test));

    CHECK(ctx_.invoke<tinvoke, std::string, int>(ind, 1) == "newdomain");
    CHECK(ctx_.invoke<tinvoke, std::string, int>(iit, 1) == "issuetoken");

    CHECK(ctx_.invoke<tinvoke, std::string, int>(ite, 1) == "test");
    // update version of `test` to 1
    CHECK(ctx_.set_version("test", 2) == 1);
    CHECK(ctx_.invoke<tinvoke, std::string, int>(ite, 2) == "test2");
}
