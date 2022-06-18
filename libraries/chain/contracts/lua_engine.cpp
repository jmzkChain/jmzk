/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/lua_engine.hpp>

#include <lua.hpp>

#include <fc/time.hpp>
#include <fc/scoped_exit.hpp>
#include <fc/io/json.hpp>

#include <jmzk/chain/config.hpp>
#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/chain/token_database.hpp>
#include <jmzk/chain/token_database_cache.hpp>
#include <jmzk/chain/contracts/lua_db.hpp>
#include <jmzk/chain/contracts/lua_json.hpp>
#include <jmzk/chain/contracts/types.hpp>
#include <jmzk/chain/contracts/abi_serializer.hpp>

namespace jmzk { namespace chain { namespace contracts {

namespace internal {

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...)      \
    try {                                                                   \
        using vtype = typename decltype(VPTR)::element_type;                \
        VPTR = tokendb_cache.template read_token<vtype>(TYPE, PREFIX, KEY); \
    }                                                                       \
    catch(token_database_exception&) {                                      \
        jmzk_THROW2(EXCEPTION, FORMAT, ##__VA_ARGS__);                       \
    }

static void
lua_hook(lua_State* L, lua_Debug* ar) {
    lua_getfield(L, LUA_REGISTRYINDEX, config::lua_start_timestamp_key);
    FC_ASSERT(lua_isnumber(L, -1));

    auto sts = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if(sts > 0) {
        auto now = fc::time_point::now().time_since_epoch();
        if(now - fc::microseconds(sts) > fc::milliseconds(config::default_lua_max_time_ms)) {
            luaL_error(L, "exceed max time allowed");
        }
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

static int
requirex(lua_State* L) {
    if(lua_gettop(L) > 1) {
        lua_settop(L, 1);
    }

    auto module = luaL_checklstring(L, -1, nullptr);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, config::lua_token_database_key);
    FC_ASSERT(lua_islightuserdata(L, -1));

    auto& tokendb_cache = *(token_database_cache*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    auto script = make_empty_cache_ptr<script_def>();
    READ_DB_TOKEN(token_type::script, std::nullopt, module, script, unknown_script_exception, "Cannot find module script: {}", module);

    auto r = luaL_loadstring(L, script->content.c_str());
    jmzk_ASSERT2(r == LUA_OK, script_load_exceptoin, "Load module '{}' script failed: {}", module, lua_tostring(L, -1));

    auto r2 = lua_pcall(L, 0, 1, 0);
    if(lua_type(L, -1) != LUA_TTABLE) {
        return luaL_error(L, "module should return a table");
    }

    return 1;
}

static lua_State*
setup_luastate(token_database_cache& tokendb_cache, int checks) {
    auto L = luaL_newstate();
    FC_ASSERT(L != nullptr);

    auto rev = fc::make_scoped_exit([L]() mutable {
        lua_close(L);
        L = nullptr;
    });

    // stop gc
    lua_gc(L, LUA_GCSTOP, 0);

    // signal for libraries to ignore env. vars.
    lua_pushboolean(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");

    // set tokendb_cache as register value
    lua_pushlightuserdata(L, (void*)&tokendb_cache);
    lua_setfield(L, LUA_REGISTRYINDEX, config::lua_token_database_key);

    // set start ts
    if(checks) {
        auto sts = fc::time_point::now().time_since_epoch().count();
        static_assert(sizeof(lua_Integer) == sizeof(sts));
        lua_pushinteger(L, sts);
    }
    else {
        lua_pushinteger(L, 0);
    }

    lua_setfield(L, LUA_REGISTRYINDEX, config::lua_start_timestamp_key);

    // open libs and set jit off and add hook function
    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    lua_sethook(L, lua_hook, LUA_MASKCALL | LUA_MASKCOUNT, config::default_lua_checkcount);

    // open db and json libraries
    luaopen_db(L);
    lua_setglobal(L, "db");

    luaopen_json(L);
    lua_setglobal(L, "json");

    // add requirex function
    lua_pushcfunction(L, requirex);
    lua_setglobal(L, "requirex");

    // push traceback function to provide custom error message
    lua_pushcfunction(L, traceback);
    assert(lua_gettop(L) == 1);

    // fetch and push loader script
    auto script = make_empty_cache_ptr<script_def>();
    READ_DB_TOKEN(token_type::script, std::nullopt, N128(.loader), script, unknown_script_exception, "Cannot find loader script");

    auto r = luaL_loadstring(L, script->content.c_str());
    jmzk_ASSERT2(r == LUA_OK, script_load_exceptoin, "Load loader script failed: {}", lua_tostring(L, -1));

    rev.cancel();
    return L;
}

}  // namespace internal

lua_engine::lua_engine() {}

bool
lua_engine::invoke_filter(const controller& control, const action& act, const script_name& script) {
    using namespace internal;

    auto& tokendb_cache = control.token_db_cache();
    
    auto ss = make_empty_cache_ptr<script_def>();
    READ_DB_TOKEN(token_type::script, std::nullopt, script, ss, unknown_script_exception,"Cannot find script: {}", script);

    auto L = internal::setup_luastate(tokendb_cache, !control.skip_trx_checks());
    assert(lua_gettop(L) == 2); // traceback, loader

    auto rev = fc::make_scoped_exit([L]() mutable {
        lua_close(L);
        L = nullptr;
    });

    // load filter script
    auto r = luaL_loadstring(L, ss->content.c_str());
    jmzk_ASSERT2(r == LUA_OK, script_load_exceptoin, "Load '{}' script failed: {}", script, lua_tostring(L, -1));
    assert(lua_gettop(L) == 3); // traceback, loader, filter

    // push action
    auto& abi = control.get_abi_serializer();
    auto  var = fc::variant();
    abi.to_variant(act, var, control.get_execution_context());

    auto json = fc::json::to_string(var);
    lua_getglobal(L, "json");
    lua_getfield(L, -1, "deserialize");
    lua_pushstring(L, json.c_str());
    
    auto r2 = lua_pcall(L, 1, 1, 1);
    jmzk_ASSERT2(r2 == LUA_OK, script_execution_exceptoin, "Convert action to json failed: {}", lua_tostring(L, -1));
    
    // remove json object from stack
    lua_copy(L, -1, -2);
    lua_pop(L, 1);
    assert(lua_gettop(L) == 4); // traceback, loader, filter, act

    // call filter
    auto r3 = lua_pcall(L, 2, 1, 1);
    jmzk_ASSERT2(r3 == LUA_OK, script_execution_exceptoin, "Lua script executed failed: {}", lua_tostring(L, -1));

    jmzk_ASSERT2(lua_gettop(L) >= 2, script_invalid_result_exceptoin, "No result is returned from script, should at least be one");
    jmzk_ASSERT2(lua_isboolean(L, -1), script_invalid_result_exceptoin, "Result returned from lua filter should be boolean value");      

    return lua_toboolean(L, -1);
}

}}}  // namespac jmzk::chain::contracts
