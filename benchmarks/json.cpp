/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <benchmark/benchmark.h>
#include <fc/io/json.hpp>

/*
 * Benchmarks for the json serizlize & deserizlize between fc library and rapidjson
 */

auto json1 = R"(
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

auto json2 = R"(
{
    "name": "testgroup",
    "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "root": {
        "threshold": 6,
        "nodes": [
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            },
            {
                "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 3
            },
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            }
        ]
    }
}
)";

static void
BM_Json_Deserialzie_FC(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    fc::variant v;

    for(auto _ : state) {
        v = fc::json::from_string(strjson, fc::json::legacy_parser);
        (void)v;
    }
}
BENCHMARK(BM_Json_Deserialzie_FC)->Arg(1)->Arg(2);

static void
BM_Json_Deserialzie_RJ(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    fc::variant v;

    for(auto _ : state) {
        v = fc::json::from_string(strjson, fc::json::rapidjson_parser);
        (void)v;
    }
}
BENCHMARK(BM_Json_Deserialzie_RJ)->Arg(1)->Arg(2);

static void
BM_Json_Serialize_FC(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    auto v = fc::json::from_string(strjson);

    for(auto _ : state) {
        auto str = fc::json::to_string(v, fc::json::stringify_large_ints_and_doubles);
        (void)str;
    }
}
BENCHMARK(BM_Json_Serialize_FC)->Arg(1)->Arg(2);

static void
BM_Json_Serialize_RJ(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    auto v = fc::json::from_string(strjson);

    for(auto _ : state) {
        auto str = fc::json::to_string(v, fc::json::rapidjson_generator);
        (void)str;
    }
}
BENCHMARK(BM_Json_Serialize_RJ)->Arg(1)->Arg(2);

static void
BM_Json_Serialize_Pretty_FC(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    auto v = fc::json::from_string(strjson);

    for(auto _ : state) {
        auto str = fc::json::to_pretty_string(v, fc::json::stringify_large_ints_and_doubles);
        (void)str;
    }
}
BENCHMARK(BM_Json_Serialize_Pretty_FC)->Arg(1)->Arg(2);

static void
BM_Json_Serialize_Pretty_RJ(benchmark::State& state) {
    auto json = state.range(0) == 1 ? json1 : json2;
    auto strjson = std::string((const char*)json);
    auto v = fc::json::from_string(strjson);

    for(auto _ : state) {
        auto str = fc::json::to_pretty_string(v, fc::json::rapidjson_generator);
        (void)str;
    }
}
BENCHMARK(BM_Json_Serialize_Pretty_RJ)->Arg(1)->Arg(2);