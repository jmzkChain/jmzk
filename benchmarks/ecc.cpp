/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <chrono>
#include <limits>
#include <random>
#include <benchmark/benchmark.h>
#include <fc/crypto/public_key.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/signature.hpp>

/*
 * Benchmarks for the ECC operations
 */

using namespace fc;
using namespace fc::crypto;

static void
BM_ECC_SignSignature(benchmark::State& state) {
    auto buf = std::string();

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

    for(auto i = 0u; i < sizeof(buf); i++) {
        buf.push_back((char)dist(dre));
    }

    auto digest = sha256::hash(buf);
    for(auto _ : state) {
        state.PauseTiming();
        auto pkey = private_key::generate();
        state.ResumeTiming();

        auto sig = pkey.sign(digest);
        (void)sig;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ECC_SignSignature);

static void
BM_ECC_VerifySignature(benchmark::State& state) {
    auto buf = std::string();

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, std::numeric_limits<char>::max());

    for(auto i = 0u; i < sizeof(buf); i++) {
        buf.push_back((char)dist(dre));
    }

    auto digest = sha256::hash(buf);
    for(auto _ : state) {
        state.PauseTiming();
        auto pkey   = private_key::generate();
        auto pubkey = pkey.get_public_key();
        auto sig    = pkey.sign(digest);
        state.ResumeTiming();

        auto pubkey2 = public_key(sig, digest);
        auto r = (pubkey2 == pubkey);
        (void)r;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ECC_VerifySignature);
