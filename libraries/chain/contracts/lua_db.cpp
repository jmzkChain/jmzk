#include <evt/chain/contracts/lua_db.hpp>

#include <fc/io/json.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/token_database_cache.hpp>
#include <evt/chain/contracts/lua_engine.hpp>
#include <evt/chain/contracts/types.hpp>

using namespace evt::chain;
using namespace evt::chain::contracts;

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...) \
    try {                                                              \
        using vtype = typename decltype(VPTR)::element_type;           \
        VPTR = db.template read_token<vtype>(TYPE, PREFIX, KEY);       \
    }                                                                  \
    catch(token_database_exception&) {                                 \
        EVT_THROW2(EXCEPTION, FORMAT, ##__VA_ARGS__);                  \
    }

extern "C" {

static int
readtoken(lua_State* L) {
    if(lua_gettop(L) > 2) {
        lua_settop(L, 2);
    }

    auto domain = luaL_checklstring(L, -2, nullptr);
    auto name   = luaL_checklstring(L, -1, nullptr);

    lua_pop(L, 2);

    lua_getfield(L, LUA_REGISTRYINDEX, config::lua_token_database_key);
    FC_ASSERT(lua_islightuserdata(L, -1));

    auto& db = *(token_database_cache*)lua_touserdata(L, -1);
    lua_pop(L, 1);

    auto token =  make_empty_cache_ptr<token_def>();
    READ_DB_TOKEN(token_type::token, domain, name, token, unknown_token_exception,"Cannot token '{}' in '{}'", name, domain);

    auto json = fc::json::to_string(fc::variant(*token));
    lua_getglobal(L, "json");
    lua_getfield(L, -1, "deserialize");
    lua_pushstring(L, json.c_str());
    
    auto r = lua_pcall(L, 1, 1, 1);
    if(r == LUA_OK) {
        return 1;
    }

    return lua_error(L);
}

static int
wrap_exceptions(lua_State *L, lua_CFunction f) {
    try {
        return f(L);
    }
    catch(const fc::unrecoverable_exception&) {
        return luaL_error(L, "unrecoverable exception");
    }
    catch(evt::chain::chain_exception& e) {
        auto err = e.what();
        return luaL_error(L, "chain error: %s", err);
    }
    catch(fc::exception& e) {
        auto err = e.what();
        return luaL_error(L, "chain error: %s", err);
    }
    catch(const std::exception& e) {
        auto err = e.what();
        return luaL_error(L, "chain error: %s", err);
    }
    catch(...) {
        return luaL_error(L, "unknown C++ error");
    }
}

static int
lreadtoken(lua_State* L) {
    return wrap_exceptions(L, readtoken);
}

int
luaopen_db(lua_State* L) {
    luaL_Reg l[] = {
        { "readtoken", lreadtoken },
        { NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

}  // extern "C"
