#include <evt/chain/contracts/lua_db.hpp>

#include <fc/io/json.hpp>
#include <evt/chain/contracts/lua_engine.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/token_database.hpp>

using namespace evt::chain;
using namespace evt::chain::contracts;

extern "C" {

static int
readtoken(lua_State* L) {
    if(lua_gettop(L) > 2) {
        lua_settop(L, 2);
    }

    auto domain = luaL_checklstring(L, -2, nullptr);
    auto name   = luaL_checklstring(L, -1, nullptr);

    lua_pop(L, 2);

    auto& db  = lua_engine::get_token_db();
    auto  str = std::string();
    auto  t   = token_def();

    db.read_token(token_type::token, domain, name, str);
    extract_db_value(str, t);

    auto json = fc::json::to_string(fc::variant(t));
    lua_getglobal(L, "json");
    lua_getfield(L, -1, "deserialize");
    lua_pushstring(L, json.c_str());
    
    auto r = lua_pcall(L, 1, 1, 0);
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
