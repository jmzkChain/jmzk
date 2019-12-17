#include <evt/chain/contracts/lua_db.hpp>
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

    lua_newtable(L);
    // domain
    lua_pushstring(L, domain);
    lua_setfield(L, -2, "domain");
    // name
    lua_pushstring(L, name);
    lua_setfield(L, -2, "name");
    // owners
    lua_newtable(L);
    int i = 1;
    for(auto& owner : t.owner) {
        auto o = (std::string)owner;
        lua_pushstring(L, o.c_str());
        lua_rawseti(L, -2, i++);
    }
    lua_setfield(L, -2, "owners");
    // metas
    lua_newtable(L);
    i = 1;
    for(auto& meta : t.metas) {
        lua_newtable(L);
        auto k = (std::string)meta.key;
        lua_pushstring(L, k.c_str());
        lua_setfield(L, -2, "key");

        lua_pushstring(L, meta.value.c_str());
        lua_setfield(L, -2, "value");

        auto c = meta.creator.to_string();
        lua_pushstring(L, c.c_str());
        lua_setfield(L, -2, "creator");

        lua_rawseti(L, -2, i++);
    }
    lua_setfield(L, -2, "metas");

    return 1;
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
