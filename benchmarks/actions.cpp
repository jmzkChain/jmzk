/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <chrono>
#include <random>
#include <benchmark/benchmark.h>
#include <fc/io/json.hpp>
#include <evt/testing/tester.hpp>
#include <evt/chain/token_database.hpp>

/*
 * Benchmarks for actions to measure the caclulation complexity
 */

using namespace evt::chain;
using namespace evt::chain::contracts;

static std::unique_ptr<evt::testing::tester>
create_tester() {
    using namespace evt::testing;

    fc::logger::get().set_log_level(fc::log_level(fc::log_level::error));

    auto dir = fc::path("/tmp/evt_benchmarks");
    if(fc::exists(dir)) {
        fc::remove_all(dir);
    }
    fc::create_directories(dir);

    auto cfg = controller::config();

    cfg.blocks_dir            = dir / "blocks";
    cfg.state_dir             = dir / "state";
    cfg.tokendb_dir           = dir / "tokendb";
    cfg.state_size            = 1024 * 1024 * 8;
    cfg.reversible_cache_size = 1024 * 1024 * 8;
    cfg.contracts_console     = false;
    cfg.charge_free_mode      = true;
    cfg.loadtest_mode         = true;

    cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
    cfg.genesis.initial_key       = tester::get_public_key("evt");
    auto privkey                  = tester::get_private_key("evt");

    auto t = std::make_unique<tester>(cfg);
    t->block_signing_private_keys.insert(std::make_pair(cfg.genesis.initial_key, privkey));

    return t;
}

static name128
get_nonce_name(const char* prefix) {
    auto n = std::string(prefix);

    auto dre = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, 25);

    n.reserve(n.size() + 10);
    for(int i = 0; i < 10; i++) {
        n.push_back('a' + dist(dre));
    }

    return name128(n);
}

const char* ndjson = R"=====(
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

static void
BM_Action_newdomain(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();
    nd.creator  = evt::testing::tester::get_public_key("evt");
    auto auths  = std::vector<account_name> { N128(evt) };

    for(auto _ : state) {
        state.PauseTiming();

        nd.name    = get_nonce_name("domain");
        auto ndact = action(nd.name, N128(.create), nd);
        
        state.ResumeTiming();

        tester->push_action(std::move(ndact), auths, address());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_newdomain);

static void
BM_Action_issuetoken(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator  = evt::testing::tester::get_public_key("evt");
    nd.name     = get_nonce_name("domain");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact  = action(nd.name, N128(.create), nd);
    auto auths  = std::vector<account_name> { N128(evt) };

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = { nd.creator };

    for(auto _ : state) {
        state.PauseTiming();

        it.names.clear();
        it.names.reserve(state.range(0));

        for(int i = 0; i < state.range(0); i++) {
            it.names.emplace_back(get_nonce_name("token"));
        }
        auto itact = action(nd.name, N128(.issue), it);

        state.ResumeTiming();

        tester->push_action(std::move(itact), auths, address());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_issuetoken)->Range(8, 8<<10);

static void
BM_Action_transfer(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator  = evt::testing::tester::get_public_key("evt");
    nd.name     = get_nonce_name("domain");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact  = action(nd.name, N128(.create), nd);
    auto auths  = std::vector<account_name> { N128(evt) };

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = { nd.creator };

    for(int i = 0; i < 1'000'000; i++) {
        it.names.emplace_back(get_nonce_name("token"));
    }
    auto itact = action(nd.name, N128(.issue), it);
    tester->push_action(std::move(itact), auths, address());

    auto tt = transfer();
    tt.domain = nd.name;
    tt.to     = { address(nd.creator) };
    tt.memo   = "";

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, 999999);

    for(auto _ : state) {
        state.PauseTiming();

        tt.name = it.names[dist(dre)];
        auto ttact = action(tt.domain, tt.name, tt);

        state.ResumeTiming();

        tester->push_action(std::move(ttact), auths, address());
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_transfer);