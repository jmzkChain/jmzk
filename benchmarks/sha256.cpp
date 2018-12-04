/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <random>
#include <chrono>
#include <limits>
#include <benchmark/benchmark.h>
#include "sha256/sha256.hpp"

static void
BM_SHA256_INTRINSICS(benchmark::State& state) {
    auto buf = std::string();

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

    for(auto i = 0u; i < 256; i++) {
        buf.push_back((char)dist(dre));
    }

    uint32_t result[8];
    for(auto _ : state) {
        sha256::intrinsics::hash(buf.data(), buf.size(), result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SHA256_INTRINSICS);

// static void
// BM_SHA256_CRYPTOPP(benchmark::State& state) {
//     auto buf = std::string();

//     auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
//     auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

//     for(auto i = 0u; i < 256; i++) {
//         buf.push_back((char)dist(dre));
//     }

//     uint32_t result[8];
//     for(auto _ : state) {
//         sha256::cryptopp::hash(buf.data(), buf.size(), result);
//     }
//     state.SetItemsProcessed(state.iterations());
// }
// BENCHMARK(BM_SHA256_CRYPTOPP);

static void
BM_SHA256_FC(benchmark::State& state) {
    auto buf = std::string();

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

    for(auto i = 0u; i < 256; i++) {
        buf.push_back((char)dist(dre));
    }

    uint32_t result[8];
    for(auto _ : state) {
        sha256::fc::hash(buf.data(), buf.size(), result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SHA256_FC);

static void
BM_SHA256_CGMINER(benchmark::State& state) {
    auto buf = std::string();

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

    for(auto i = 0u; i < 256; i++) {
        buf.push_back((char)dist(dre));
    }

    uint32_t result[8];
    for(auto _ : state) {
        sha256::cgminer::hash(buf.data(), buf.size(), result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SHA256_CGMINER);
