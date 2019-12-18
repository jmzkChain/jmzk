/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/lua_engine.hpp>

#include <fc/time.hpp>
#include <evt/chain/config.hpp>
#include <lua.hpp>

namespace evt { namespace chain { namespace contracts {

namespace internal {

thread_local void*   G_TOKENDB = nullptr;
thread_local int64_t G_STARTTS = 0;

const char* LOADER = R"===(
    
)===";


static void
lua_hook(lua_State* L, lua_Debug* ar) {
    FC_ASSERT(G_STARTTS > 0);

    auto now = fc::time_point::now().time_since_epoch();
    if(now - fc::microseconds(G_STARTTS) > fc::milliseconds(config::default_lua_max_time_ms)) {
        luaL_error(L, "exceed max time allowed");
    }
}

static lua_State*
setup_luastate() {
    auto L = luaL_newstate();
    FC_ASSERT(L != nullptr);

    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    lua_sethook(L, lua_hook, LUA_MASKCALL | LUA_MASKCOUNT, config::default_lua_checkcount);

    return L;
}

}  // namespace internal

void
lua_engine::invoke_filter(const token_database& tokendb, const action& act, const std::string& filter_fuc) {
    using namespace internal;

    G_TOKENDB = (void*)&tokendb;
    G_STARTTS = fc::time_point::now().time_since_epoch().count();
    
    auto L = internal::setup_luastate();
    
    
}

void
lua_engine::set_token_db(const token_database& tokendb) {
    using namespace internal;

    G_TOKENDB = (void*)&tokendb;
}

const token_database&
lua_engine::get_token_db() {
    using namespace internal;

    assert(G_TOKENDB != nullptr);
    return *(const token_database*)G_TOKENDB;
}

}}}  // namespac evt::chain::contracts
