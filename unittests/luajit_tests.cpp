#include <catch/catch.hpp>

#include <lua.hpp>
#include <fc/io/json.hpp>
#include <jmzk/testing/tester.hpp>
#include <jmzk/chain/token_database_cache.hpp>
#include <jmzk/chain/contracts/lua_engine.hpp>
#include <jmzk/chain/contracts/lua_db.hpp>
#include <jmzk/chain/contracts/lua_json.hpp>

extern "C" {

int __attribute__ ((visibility("default"))) __attribute__ ((noinline))
ladd(int a, int b) {
    return a + b;
}

}  // extern "C"

static void
lua_hook(lua_State* L, lua_Debug* ar) {
    static int count = 0;

    lua_getinfo(L, "nS", ar);
    printf("hook %d from %s: %s %s\n", ar->event, ar->what, ar->name, ar->namewhat);

    if(++count == 10) {
        luaL_error(L, "exceed hook count");
    }
}

static int
traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if(msg) {
        luaL_traceback(L, L, msg, 1);
    }
    else {
        lua_pushliteral(L, "(no error message)");
    }
    return 1;
}


TEST_CASE("test_lua_debug", "[luajit]") {
    auto L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    lua_sethook(L, lua_hook, LUA_MASKCALL | LUA_MASKCOUNT, 100);

    auto script = R"===(
        local ffi = require("ffi")
        ffi.cdef[[
        int ladd(int a, int b);
        ]]


        local function add(a, b)
            return a + b
        end

        local i = 0
        while true do
            i = add(i, 1)
            i = ffi.C.ladd(i, 1)

            if i == 1000 then
                break
            end
        end
    )===";

    auto r = luaL_loadstring(L, script);
    REQUIRE(r == LUA_OK);

    auto r2 = lua_pcall(L, 0, 0, 0);
    REQUIRE(r2 == LUA_ERRRUN);

    printf("error: %s\n", lua_tostring(L,-1));

    int a = 1, b = 2;
    int c = ladd(a, b);
    REQUIRE(c == 3);
}

extern std::string jmzk_unittests_dir;

TEST_CASE("test_lua_db", "[luajit]") {
    using namespace jmzk::testing;
    using namespace jmzk::chain::contracts;

    auto basedir = jmzk_unittests_dir + "/tokendb_tests";
    if(!fc::exists(basedir)) {
        fc::create_directories(basedir);
    }

    auto cfg = jmzk::chain::controller::config();
    cfg.blocks_dir            = basedir + "/blocks";
    cfg.state_dir             = basedir + "/state";
    cfg.db_config.db_path     = basedir + "/tokendb";
    cfg.contracts_console     = true;
    cfg.charge_free_mode      = false;
    cfg.loadtest_mode         = false;

    cfg.genesis.initial_timestamp = fc::time_point::now();
    cfg.genesis.initial_key       = tester::get_public_key("jmzk");
    
    auto mytester = std::make_unique<tester>(cfg);
    auto& tokendb = mytester->control->token_db();

    const char* test_data = R"=====(
    {
        "domain": "tkdomain",
        "name": "tktoken",
        "owner": [
          "jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
          "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
        ],
        "metas": [
            { "key": "tm1", "value": "hello1", "creator": "[A] jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
            { "key": "tm2", "value": "hello2", "creator": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2" }
        ]
    }
    )=====";

    {
        auto vt = fc::json::from_string(test_data);
        auto tt = token_def();
        fc::from_variant(vt, tt);
        auto dt = make_db_value(tt);
        tokendb.put_token(token_type::token, action_op::put, tt.domain, tt.name, dt.as_string_view());
    }
    
    auto L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);

    luaopen_db(L);
    lua_setglobal(L, "db");

    luaopen_json(L);
    lua_setglobal(L, "json");
    
    // push traceback function to provide custom error message
    lua_pushcfunction(L, traceback);
    assert(lua_gettop(L) == 1);

    auto tokendb_cache = mytester->control->token_db_cache();
    lua_pushlightuserdata(L, (void*)&tokendb_cache);
    lua_setfield(L, LUA_REGISTRYINDEX, config::lua_token_database_key);

    auto script = R"===(
        local t = db.readtoken("tkdomain", "tktoken")
        local jt = json.serialize(t)
        print(t, jt)
    )===";

    auto r = luaL_loadstring(L, script);
    REQUIRE(r == LUA_OK);

    auto r2 = lua_pcall(L, 0, 0, 0);
    CHECK(r2 == LUA_OK);

    if(r2 != LUA_OK) {
        printf("error: %s\n", lua_tostring(L,-1));
    }
}

TEST_CASE("test_lua_engine", "[luajit]") {
    using namespace jmzk::testing;
    using namespace jmzk::chain::contracts;

    auto basedir = jmzk_unittests_dir + "/tokendb_tests";
    if(!fc::exists(basedir)) {
        fc::create_directories(basedir);
    }

    auto cfg = jmzk::chain::controller::config();
    cfg.blocks_dir            = basedir + "/blocks";
    cfg.state_dir             = basedir + "/state";
    cfg.db_config.db_path     = basedir + "/tokendb";
    cfg.contracts_console     = true;
    cfg.charge_free_mode      = false;
    cfg.loadtest_mode         = false;

    cfg.genesis.initial_timestamp = fc::time_point::now();
    cfg.genesis.initial_key       = tester::get_public_key("jmzk");
    
    auto mytester = std::make_unique<tester>(cfg);
    auto& tokendb = mytester->control->token_db_cache();

    const char* test_data = R"====(
    {
        "domain": "tkdomain",
        "name": "tktoken",
        "owner": [
          "jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2",
          "jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
        ],
        "metas": [
            { "key": "tm1", "value": "hello1", "creator": "[A] jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV" },
            { "key": "tm2", "value": "hello2", "creator": "[A] jmzk5ve9Ezv9vLZKp1NmRzvB5ZoZ21YZ533BSB2Ai2jLzzMep6biU2" }
        ]
    }
    )====";

    const char* loader = R"===(
        local filter_fn, act = ...

        return filter_fn(act)
    )===";

    const char* script = R"===(
        local act = ...
        if act.name ~= 'transferft' then
            error('only transferft is allowed')
        end
        
        if act.data.memo == 'haha' then
            error('invalid memo')
        end

        local token = db.readtoken(act.domain, act.key)
        for i, v in ipairs(token.metas) do
            if v.key == 'tm3' then
                error('meta key with tm3 is not allowed to be transferred')
            end
        end

        return true
    )===";

    const char* script2 = R"===(
        local i = 0
        while true do
            i = i + 1
        end
    )===";

    const char* script3 = R"===(
        local haha = {}

        function haha.add(a, b)
            return a + b
        end

        return haha
    )===";

    const char* script4 = R"===(
        local haha = {}

        function haha.add(a, b)
            return a + b
        end
    )===";

    const char* script5 = R"===(
        local haha = requirex("script3")
        if haha.add(1, 2) == 3 then
            return true
        end
        return false
    )===";

    const char* script6 = R"===(
        local haha = requirex("script4")
        if haha.add(1, 2) == 3 then
            return true
        end
        return false
    )===";

    auto vt = fc::json::from_string(test_data);
    auto tt = token_def();
    fc::from_variant(vt, tt);
    auto ptt = tokendb.put_token<token_def&, true>(token_type::token, action_op::put, tt.domain, tt.name, tt);

    auto add_script = [&](auto name, auto script) {
        auto sl = script_def();
        sl.content = script;
        tokendb.put_token(token_type::script, action_op::put, std::nullopt, name, sl);
    };

    add_script(".loader", loader);
    add_script("script",  script);
    add_script("script2", script2);
    add_script("script3", script3);
    add_script("script4", script4);
    add_script("script5", script5);
    add_script("script6", script6);

    auto engine = lua_engine();

    auto tf = transferft();
    tf.memo = "haha";
    auto act = action("tkdomain", "tktoken", tf);
    // memo is invalid
    CHECK_THROWS_AS(engine.invoke_filter(*mytester->control, act, "script"), script_execution_exceptoin);

    tf.memo = "lala";
    act = action("tkdomain", "tktoken", tf);

    ptt->metas.push_back(meta("tm3", "nonce", authorizer_ref()));
    tokendb.put_token(token_type::token, action_op::put, tt.domain, tt.name, *ptt);
    // metadata is invalid
    CHECK_THROWS_AS(engine.invoke_filter(*mytester->control, act, "script"), script_execution_exceptoin);

    ptt->metas.erase(ptt->metas.end() - 1);
    tokendb.put_token(token_type::token, action_op::put, tt.domain, tt.name, *ptt);
    CHECK_NOTHROW(engine.invoke_filter(*mytester->control, act, "script"));

    CHECK_THROWS_AS(engine.invoke_filter(*mytester->control, act, "script2"), script_execution_exceptoin);

    CHECK_THROWS_AS(engine.invoke_filter(*mytester->control, act, "script6"), script_execution_exceptoin);
    CHECK_NOTHROW(engine.invoke_filter(*mytester->control, act, "script5"));
}
