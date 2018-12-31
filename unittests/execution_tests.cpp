#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include <vector>

#include <catch/catch.hpp>

#include <fc/io/json.hpp>

#include <evt/chain/contracts/types.hpp>
#include <evt/chain/execution/execution_context.hpp>

using namespace evt::chain;

using execution::execution_context;
using execution_context_test = execution_context<
                                   contracts::newdomain,
                                   contracts::updatedomain,
                                   contracts::issuetoken,
                                   contracts::transfer
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