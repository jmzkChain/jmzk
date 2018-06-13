/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <benchmark/benchmark.h>
#include <fc/io/json.hpp>

/*
 * Benchmarks for the json serizlize & deserizlize between fc library and rapidjson
 */

auto json1 = R"(
{
    "name": "test",
    "issuer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
    "issue": {
        "name": "issue",
        "threshold": 1,
        "authorizers": [{
            "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
            "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
            "weight": 1
        }]
    }
}
)";

auto json2 = R"(
{
    "name": "testgroup",
    "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "root": {
        "threshold": 6,
        "nodes": [
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            },
            {
                "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 3
            },
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            }
        ]
    }
}
)";

static void
BM_Json_Serialzie_FC(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    fc::variant v;

    for(auto _ : state) {
        v = fc::json::from_string(strjson);
        (void)v;
    }
}
BENCHMARK(BM_Json_Serialzie_FC)->Arg(1)->Arg(2);
