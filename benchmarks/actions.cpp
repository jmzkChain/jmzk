/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <benchmark/benchmark.h>
#include <chrono>
#include <evt/chain/token_database.hpp>
#include <evt/chain/transaction_context.hpp>
#include <evt/testing/tester.hpp>
#include <fc/io/json.hpp>
#include <random>

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

std::string
get_nonce_sym() {
    static auto dre = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());

    auto dist = std::uniform_int_distribution<uint32_t>(0, std::numeric_limits<uint32_t>::max());

    return std::to_string(dist(dre));
}

static name128
get_nonce_name(const char* prefix) {
    static auto dre = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());

    auto n    = std::string(prefix);
    auto dist = std::uniform_int_distribution<int>(0, 25);

    n.reserve(n.size() + 10);
    for(int i = 0; i < 10; i++) {
        n.push_back('a' + dist(dre));
    }

    return name128(n);
}

static transaction_metadata
get_trx_meta(controller& control, const action& act, const std::vector<account_name>& auths) {
    auto signed_trx = signed_transaction();
    signed_trx.actions.emplace_back(act);

    for(auto& auth : auths) {
        auto privkey = evt::testing::tester::get_private_key(auth);
        signed_trx.sign(privkey, control.get_chain_id());
    }

    return transaction_metadata(signed_trx);
}

static transaction_context
get_trx_ctx(controller& control, transaction_metadata& trx_meta) {
    return transaction_context(control, trx_meta);
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

const char* ngjson = R"=====(
{
  "name" : "5jxX",
  "group" : {
    "name": "5jxXg",
    "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "root": {
      "threshold": 6,
      "weight": 0,
      "nodes": [{
          "type": "branch",
          "threshold": 1,
          "weight": 3,
          "nodes": [{
              "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
              "weight": 1
            },{
              "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
              "weight": 1
            }
          ]
        },{
          "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
          "weight": 3
        },{
          "threshold": 1,
          "weight": 3,
          "nodes": [{
              "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
              "weight": 1
            },{
              "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
              "weight": 2
            }
          ]
        }
      ]
    }
  }
}
)=====";

const char* nfjson = R"=====(
{
  "name": "EVT",
  "sym_name": "EVT",
  "sym": "5,S#3",
  "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "issue" : {
    "name" : "issue",
    "threshold" : 1,
    "authorizers": [{
        "ref": "[A] EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
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
  },
  "total_supply":"12.00000 S#3"
}
)=====";

static void
BM_Action_newdomain(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();
    nd.creator  = evt::testing::tester::get_public_key("evt");
    auto auths  = std::vector<account_name>{N128(evt)};

    for(auto _ : state) {
        state.PauseTiming();

        nd.name       = get_nonce_name("domain");
        auto ndact    = action(nd.name, N128(.create), nd);
        auto trx_meta = get_trx_meta(*tester->control, ndact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_newdomain);

static void
BM_Action_updatedomain(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator = evt::testing::tester::get_public_key("evt");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact = action(nd.name, N128(.create), nd);
    auto auths = std::vector<account_name>{N128(evt)};

    auto ud     = updatedomain();
    ud.name     = nd.name;
    ud.issue    = nd.issue;
    ud.transfer = nd.transfer;
    ud.manage   = nd.manage;

    for(auto _ : state) {
        state.PauseTiming();
        nd.name = get_nonce_name("domain");
        ndact   = action(nd.name, N128(.create), nd);
        tester->push_action(std::move(ndact), auths, address());

        ud.name       = nd.name;
        auto ndact    = action(ud.name, N128(.update), ud);
        auto trx_meta = get_trx_meta(*tester->control, ndact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_updatedomain);

static void
BM_Action_issuetoken(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator = evt::testing::tester::get_public_key("evt");
    nd.name    = get_nonce_name("domain");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact = action(nd.name, N128(.create), nd);
    auto auths = std::vector<account_name>{N128(evt)};

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = {nd.creator};

    for(auto _ : state) {
        state.PauseTiming();

        it.names.clear();
        it.names.reserve(state.range(0));

        for(int i = 0; i < state.range(0); i++) {
            it.names.emplace_back(get_nonce_name("token"));
        }
        auto itact    = action(nd.name, N128(.issue), it);
        auto trx_meta = get_trx_meta(*tester->control, itact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_issuetoken)->Range(1, 8 << 10);

static void
BM_Action_transfer(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator = evt::testing::tester::get_public_key("evt");
    nd.name    = get_nonce_name("domain");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact = action(nd.name, N128(.create), nd);
    auto auths = std::vector<account_name>{N128(evt)};

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = {nd.creator};

    for(int i = 0; i < 1'000'000; i++) {
        it.names.emplace_back(get_nonce_name("token"));
    }
    auto itact = action(nd.name, N128(.issue), it);
    tester->push_action(std::move(itact), auths, address());

    auto tt   = transfer();
    tt.domain = nd.name;
    tt.to     = {address(nd.creator)};
    tt.memo   = "";

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, 999999);

    for(auto _ : state) {
        state.PauseTiming();

        tt.name       = it.names[dist(dre)];
        auto ttact    = action(tt.domain, tt.name, tt);
        auto trx_meta = get_trx_meta(*tester->control, ttact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_transfer);

static void
BM_Action_destroytoken(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();

    nd.creator = evt::testing::tester::get_public_key("evt");
    nd.name    = get_nonce_name("domain");
    nd.issue.authorizers[0].ref.set_account(nd.creator);

    auto ndact = action(nd.name, N128(.create), nd);
    auto auths = std::vector<account_name>{N128(evt)};

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = {nd.creator};

    for(int i = 0; i < 1'000'000; i++) {
        it.names.emplace_back(get_nonce_name("token"));
    }
    auto itact = action(nd.name, N128(.issue), it);
    tester->push_action(std::move(itact), auths, address());

    auto dt   = destroytoken();
    dt.domain = nd.name;

    int index = 0;
    for(auto _ : state) {
        state.PauseTiming();

        dt.name       = it.names[index++];
        auto dtact    = action(dt.domain, dt.name, dt);
        auto trx_meta = get_trx_meta(*tester->control, dtact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_destroytoken);

static void
BM_Action_newgroup(benchmark::State& state) {
    auto tester   = create_tester();
    auto var      = fc::json::from_string(ngjson);
    auto ng       = var.as<newgroup>();
    ng.group.key_ = evt::testing::tester::get_public_key("evt");
    auto auths    = std::vector<account_name>{N128(evt)};

    for(auto _ : state) {
        state.PauseTiming();

        ng.name        = get_nonce_name("group");
        ng.group.name_ = ng.name;
        auto ngact     = action(N128(.group), ng.name, ng);
        auto trx_meta  = get_trx_meta(*tester->control, ngact, auths);
        auto trx_ctx   = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_newgroup);

static void
BM_Action_updategroup(benchmark::State& state) {
    auto tester    = create_tester();
    auto var       = fc::json::from_string(ngjson);
    auto ng        = var.as<newgroup>();
    ng.group.key_  = evt::testing::tester::get_public_key("evt");
    ng.name        = get_nonce_name("group");
    ng.group.name_ = ng.name;
    auto ngact     = action(N128(.group), ng.name, ng);
    auto auths     = std::vector<account_name>{N128(evt)};

    auto ug = updategroup();

    for(auto _ : state) {
        state.PauseTiming();
        ng.name        = get_nonce_name("group");
        ng.group.name_ = ng.name;
        auto ngact     = action(N128(.group), ng.name, ng);
        tester->push_action(std::move(ngact), auths, address());

        ug.name       = ng.name;
        ug.group      = ng.group;
        auto ugact    = action(N128(.group), ug.name, ug);
        auto trx_meta = get_trx_meta(*tester->control, ugact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_updategroup);

static void
BM_Action_newfungible(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(nfjson);
    auto nf     = var.as<newfungible>();
    nf.creator  = evt::testing::tester::get_public_key("evt");
    auto auths  = std::vector<account_name>{N128(evt)};
    for(auto _ : state) {
        state.PauseTiming();

        nf.name         = get_nonce_sym();
        nf.sym_name     = get_nonce_sym();
        nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
        nf.total_supply = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
        auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
        auto trx_meta   = get_trx_meta(*tester->control, nfact, auths);
        auto trx_ctx    = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_newfungible);

static void
BM_Action_updfungible(benchmark::State& state) {
    auto tester     = create_tester();
    auto var        = fc::json::from_string(nfjson);
    auto nf         = var.as<newfungible>();
    nf.creator      = evt::testing::tester::get_public_key("evt");
    nf.name         = get_nonce_sym();
    nf.sym_name     = get_nonce_sym();
    nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
    nf.total_supply = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
    auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
    auto auths      = std::vector<account_name>{N128(evt)};
    tester->push_action(std::move(nfact), auths, address());

    auto uf   = updfungible();
    uf.sym_id = nf.sym.id();
    uf.issue  = nf.issue;
    uf.manage = nf.manage;
    for(auto _ : state) {
        state.PauseTiming();
        nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
        nf.total_supply = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
        auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
        tester->push_action(std::move(nfact), auths, address());

        uf.sym_id     = nf.sym.id();
        auto ufact    = action(N128(.fungible), (name128)std::to_string(uf.sym_id), uf);
        auto trx_meta = get_trx_meta(*tester->control, ufact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_updfungible);

static void
BM_Action_issuefungible(benchmark::State& state) {
    auto tester     = create_tester();
    auto var        = fc::json::from_string(nfjson);
    auto nf         = var.as<newfungible>();
    nf.creator      = evt::testing::tester::get_public_key("evt");
    nf.name         = get_nonce_sym();
    nf.sym_name     = get_nonce_sym();
    nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
    nf.total_supply = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
    auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
    auto auths      = std::vector<account_name>{N128(evt)};

    tester->push_action(std::move(nfact), auths, address());

    auto isf   = issuefungible();
    isf.number = asset::from_string(string("0.00001 S#") + std::to_string(nf.sym.id()));

    for(auto _ : state) {
        state.PauseTiming();

        isf.address = evt::testing::tester::get_public_key(get_nonce_name(""));

        auto isfact   = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), isf);
        auto trx_meta = get_trx_meta(*tester->control, isfact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_issuefungible);

static void
BM_Action_transferft(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(nfjson);
    auto nf     = var.as<newfungible>();
    nf.creator  = evt::testing::tester::get_public_key("evt");
    nf.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    nf.name         = get_nonce_sym();
    nf.sym_name     = get_nonce_sym();
    nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
    nf.total_supply = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
    auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
    auto auths      = std::vector<account_name>{N128(evt)};

    tester->push_action(std::move(nfact), auths, address());

    auto isf    = issuefungible();
    isf.number  = asset::from_string(string("100.00000 S#") + std::to_string(nf.sym.id()));
    isf.address = address(evt::testing::tester::get_public_key("evt"));
    auto isfact = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), isf);

    tester->push_action(std::move(isfact), auths, address());

    auto tf   = transferft();
    tf.from   = evt::testing::tester::get_public_key("evt");
    tf.number = asset::from_string(string("0.00001 S#") + std::to_string(nf.sym.id()));

    for(auto _ : state) {
        state.PauseTiming();

        tf.to = evt::testing::tester::get_public_key(get_nonce_name(""));

        auto tfact    = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), tf);
        auto trx_meta = get_trx_meta(*tester->control, tfact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_transferft);

static void
BM_Action_evt2pevt(benchmark::State& state) {
    auto tester = create_tester();

    auto auths = std::vector<account_name>{N128(evt)};
    tester->add_money(address(evt::testing::tester::get_public_key(N(evt))), asset(10000000, evt_sym()));

    auto e2p   = evt2pevt();
    e2p.from   = address(evt::testing::tester::get_public_key(N(evt)));
    e2p.number = asset::from_string(string("0.00001 S#1"));

    for(auto _ : state) {
        state.PauseTiming();

        e2p.to = evt::testing::tester::get_public_key(get_nonce_name(""));

        auto e2pact   = action(N128(.fungible), std::to_string(evt_sym().id()), e2p);
        auto trx_meta = get_trx_meta(*tester->control, e2pact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_evt2pevt);

static void
BM_Action_fungible_addmeta(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(nfjson);
    auto nf     = var.as<newfungible>();
    nf.creator  = evt::testing::tester::get_public_key("evt");
    nf.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    nf.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    auto auths      = std::vector<account_name>{N128(evt)};

    auto am    = addmeta();
    am.creator = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();
        nf.sym          = symbol::from_string(string("5,S#") + get_nonce_sym());
        nf.total_supply = asset::from_string(string("100000.00000 S#") + std::to_string(nf.sym.id()));
        auto nfact      = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), nf);
        tester->push_action(std::move(nfact), auths, address());

        am.key   = get_nonce_name("key");
        am.value = get_nonce_sym();

        auto amact    = action(N128(.fungible), (name128)std::to_string(nf.sym.id()), am);
        auto trx_meta = get_trx_meta(*tester->control, amact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_fungible_addmeta);

static void
BM_Action_group_addmeta(benchmark::State& state) {
    auto tester    = create_tester();
    auto var       = fc::json::from_string(ngjson);
    auto ng        = var.as<newgroup>();
    ng.group.key_  = evt::testing::tester::get_public_key("evt");
    auto auths     = std::vector<account_name>{N128(evt)};

    auto am    = addmeta();
    am.creator = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();
        ng.name        = get_nonce_name("group");
        ng.group.name_ = ng.name;
        auto ngact     = action(N128(.group), ng.name, ng);
        tester->push_action(std::move(ngact), auths, address());

        am.key   = get_nonce_name("key");
        am.value = get_nonce_sym();

        auto amact    = action(N128(.group), ng.name, am);
        auto trx_meta = get_trx_meta(*tester->control, amact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_group_addmeta);

static void
BM_Action_domain_addmeta(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();
    nd.creator  = evt::testing::tester::get_public_key("evt");
    nd.manage.authorizers[0].ref.set_account(nd.creator);
    auto auths = std::vector<account_name>{N128(evt)};

    auto am    = addmeta();
    am.creator = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();
        nd.name    = get_nonce_name("domain");
        auto ndact = action(nd.name, N128(.create), nd);

        tester->push_action(std::move(ndact), auths, address());

        am.key   = get_nonce_name("key");
        am.value = get_nonce_sym();

        auto amact    = action(name128(nd.name), N128(.meta), am);
        auto trx_meta = get_trx_meta(*tester->control, amact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_domain_addmeta);

static void
BM_Action_token_addmeta(benchmark::State& state) {
    auto tester = create_tester();
    auto var    = fc::json::from_string(ndjson);
    auto nd     = var.as<newdomain>();
    nd.creator  = evt::testing::tester::get_public_key("evt");
    nd.issue.authorizers[0].ref.set_account(nd.creator);
    auto auths = std::vector<account_name>{N128(evt)};
    nd.name    = get_nonce_name("domain");
    auto ndact = action(nd.name, N128(.create), nd);

    tester->push_action(std::move(ndact), auths, address());

    auto it   = issuetoken();
    it.domain = nd.name;
    it.owner  = {nd.creator};

    for(int i = 0; i < 1'000'000; i++) {
        it.names.emplace_back(get_nonce_name("token"));
    }
    auto itact = action(name128(nd.name), N128(.issue), it);
    tester->push_action(std::move(itact), auths, address());

    auto dre  = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count());
    auto dist = std::uniform_int_distribution<int>(0, 999999);

    auto am    = addmeta();
    am.creator = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();

        am.key   = get_nonce_name("key");
        am.value = get_nonce_sym();

        auto amact    = action(name128(nd.name), it.names[dist(dre)], am);
        auto trx_meta = get_trx_meta(*tester->control, amact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_token_addmeta);

static void
BM_Action_newsuspend(benchmark::State& state) {
    auto        tester    = create_tester();
    const char* test_data = R"=======(
      {
          "name": "testsuspend",
          "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
          "trx": {
              "expiration": "2021-07-04T05:14:12",
              "ref_block_num": "3432",
              "ref_block_prefix": "291678901",
              "actions": [
              ],
              "transaction_extensions": []
          }
      }
      )=======";

    auto var    = fc::json::from_string(test_data);
    auto ns     = var.as<newsuspend>();
    ns.proposer = evt::testing::tester::get_public_key("evt");

    auto auths = std::vector<account_name>{N128(evt)};

    for(auto _ : state) {
        state.PauseTiming();

        ns.name            = get_nonce_name("suspend");
        auto newdomain_var = fc::json::from_string(ndjson);
        auto newdom        = newdomain_var.as<newdomain>();
        newdom.name        = get_nonce_name("");
        newdom.creator     = evt::testing::tester::get_public_key("evt");
        newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            ns.trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }

        auto nsact    = action(N128(.suspend), ns.name, ns);
        auto trx_meta = get_trx_meta(*tester->control, nsact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_newsuspend)->Range(1, 8 << 10);

static void
BM_Action_newsuspend_serialiazition(benchmark::State& state) {
    auto        tester    = create_tester();
    const char* test_data = R"=======(
      {
          "name": "testsuspend",
          "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
          "trx": {
              "expiration": "2021-07-04T05:14:12",
              "ref_block_num": "3432",
              "ref_block_prefix": "291678901",
              "actions": [
              ],
              "transaction_extensions": []
          }
      }
      )=======";

    auto var    = fc::json::from_string(test_data);
    auto ns     = var.as<newsuspend>();
    ns.proposer = evt::testing::tester::get_public_key("evt");

    auto auths = std::vector<account_name>{N128(evt)};

    for(auto _ : state) {
        state.PauseTiming();

        ns.name            = get_nonce_name("suspend");
        auto newdomain_var = fc::json::from_string(ndjson);
        auto newdom        = newdomain_var.as<newdomain>();
        newdom.name        = get_nonce_name("");
        newdom.creator     = evt::testing::tester::get_public_key("evt");
        newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            ns.trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }

        auto nsact = action(N128(.suspend), ns.name, ns);
        state.ResumeTiming();

        nsact.data_as<newsuspend>();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_newsuspend_serialiazition)->Range(1, 8 << 10);

static void
BM_Action_cancelsuspend(benchmark::State& state) {
    auto        tester    = create_tester();
    const char* test_data = R"=======(
      {
          "name": "testsuspend",
          "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
          "trx": {
              "expiration": "2021-07-04T05:14:12",
              "ref_block_num": "3432",
              "ref_block_prefix": "291678901",
              "actions": [
              ],
              "transaction_extensions": []
          }
      }
      )=======";

    auto var    = fc::json::from_string(test_data);
    auto ns     = var.as<newsuspend>();
    ns.proposer = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();

        ns.name            = get_nonce_name("suspend");
        auto newdomain_var = fc::json::from_string(ndjson);
        auto newdom        = newdomain_var.as<newdomain>();
        newdom.creator     = evt::testing::tester::get_public_key("evt");
        newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            ns.trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }
        auto nsact = action(N128(.suspend), ns.name, ns);
        auto auths = std::vector<account_name>{N128(evt)};

        tester->push_action(std::move(nsact), auths, address());

        auto cs = cancelsuspend();
        cs.name = ns.name;

        auto csact    = action(N128(.suspend), cs.name, cs);
        auto trx_meta = get_trx_meta(*tester->control, csact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_cancelsuspend)->Range(1, 8 << 5);

static void
BM_Action_aprvsuspend(benchmark::State& state) {
    auto        tester    = create_tester();
    const char* test_data = R"=======(
      {
          "name": "testsuspend",
          "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
          "trx": {
              "expiration": "2021-07-04T05:14:12",
              "ref_block_num": "3432",
              "ref_block_prefix": "291678901",
              "actions": [
              ],
              "transaction_extensions": []
          }
      }
      )=======";

    auto var    = fc::json::from_string(test_data);
    auto ns     = var.as<newsuspend>();
    ns.proposer = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();

        ns.name    = get_nonce_name("suspend");
        auto auths = std::vector<account_name>{N128(evt)};

        auto newdomain_var = fc::json::from_string(ndjson);
        auto newdom        = newdomain_var.as<newdomain>();
        newdom.name        = get_nonce_name("");
        newdom.creator     = evt::testing::tester::get_public_key("evt");
        newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            ns.trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }
        auto nsact = action(N128(.suspend), ns.name, ns);

        tester->push_action(std::move(nsact), auths, address());

        auto as       = aprvsuspend();
        as.name       = ns.name;
        auto sig      = evt::testing::tester::get_private_key("evt").sign(ns.trx.sig_digest(tester->control->get_chain_id()));
        as.signatures = {sig};

        auto asact    = action(N128(.suspend), as.name, as);
        auto trx_meta = get_trx_meta(*tester->control, asact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_aprvsuspend)->Range(1, 8 << 10);

static void
BM_Action_execsuspend(benchmark::State& state) {
    auto        tester    = create_tester();
    const char* test_data = R"=======(
      {
          "name": "testsuspend",
          "proposer": "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3",
          "trx": {
              "expiration": "2021-07-04T05:14:12",
              "ref_block_num": "3432",
              "ref_block_prefix": "291678901",
              "actions": [
              ],
              "transaction_extensions": []
          }
      }
      )=======";

    auto var    = fc::json::from_string(test_data);
    auto ns     = var.as<newsuspend>();
    ns.proposer = evt::testing::tester::get_public_key("evt");

    for(auto _ : state) {
        state.PauseTiming();

        ns.name    = get_nonce_name("suspend");
        auto auths = std::vector<account_name>{N128(evt)};

        auto newdomain_var = fc::json::from_string(ndjson);
        auto newdom        = newdomain_var.as<newdomain>();
        newdom.name        = get_nonce_name("");
        newdom.creator     = evt::testing::tester::get_public_key("evt");
        newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
        newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            ns.trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }
        auto nsact = action(N128(.suspend), ns.name, ns);

        tester->push_action(std::move(nsact), auths, address());

        auto as       = aprvsuspend();
        as.name       = ns.name;
        auto sig      = evt::testing::tester::get_private_key("evt").sign(ns.trx.sig_digest(tester->control->get_chain_id()));
        as.signatures = {sig};

        auto asact = action(N128(.suspend), as.name, as);
        tester->push_action(std::move(asact), auths, address());

        auto es     = execsuspend();
        es.name     = ns.name;
        es.executor = evt::testing::tester::get_public_key("evt");

        auto esact    = action(N128(.suspend), es.name, es);
        auto trx_meta = get_trx_meta(*tester->control, esact, auths);
        auto trx_ctx  = get_trx_ctx(*tester->control, trx_meta);

        trx_ctx.init_for_implicit_trx();

        state.ResumeTiming();

        trx_ctx.exec();
        trx_ctx.squash();
    }
    state.SetItemsProcessed(state.iterations());

    state.SetLabel(std::string("total: ") + std::to_string(state.iterations() * state.range(0)));
}
BENCHMARK(BM_Action_execsuspend)->Range(1, 8 << 10);

static void
BM_Action_get_signature_keys(benchmark::State& state) {
    auto tester = create_tester();

    auto newdomain_var = fc::json::from_string(ndjson);
    auto newdom        = newdomain_var.as<newdomain>();
    newdom.name        = get_nonce_name("");
    newdom.creator     = evt::testing::tester::get_public_key("evt");

    newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

    auto trx = transaction();

    for(auto _ : state) {
        state.PauseTiming();

        auto sigs = fc::small_vector<signature_type, 4>();
        sigs.reserve(state.range(1));

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }

        auto chain_id = tester->control->get_chain_id();
        auto digest   = trx.sig_digest(chain_id);
        
        for(int i = 0; i < state.range(1); i++) {
            auto pkey = private_key_type::generate();
            sigs.emplace_back(pkey.sign(digest));
        }

        state.ResumeTiming();

        auto signed_keys = trx.get_signature_keys(sigs, chain_id);;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_get_signature_keys)->Ranges({{1, 8 << 4}, {1, 8}});

static void
BM_Action_trx_sig_digest(benchmark::State& state) {
    auto tester = create_tester();

    auto newdomain_var = fc::json::from_string(ndjson);
    auto newdom        = newdomain_var.as<newdomain>();
    newdom.name        = get_nonce_name("");
    newdom.creator     = evt::testing::tester::get_public_key("evt");

    newdom.issue.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    newdom.manage.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));
    newdom.transfer.authorizers[0].ref.set_account(evt::testing::tester::get_public_key("evt"));

    auto trx = transaction();

    for(auto _ : state) {
        state.PauseTiming();

        for(int i = 0; i < state.range(0); i++) {
            newdom.name = get_nonce_name("");
            trx.actions.push_back(action(newdom.name, N128(.create), newdom));
        }

        auto chain_id = tester->control->get_chain_id();

        state.ResumeTiming();

        auto digest = trx.sig_digest(chain_id);
        (void)digest;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Action_trx_sig_digest)->Range(1, 8 << 10);
