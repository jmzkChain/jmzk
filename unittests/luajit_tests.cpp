#include <catch/catch.hpp>

#include <lua.hpp>

static void lua_hook(lua_State* L, lua_Debug* ar) {
    printf("hello\n");
    luaL_error(L, "too long");
}

TEST_CASE("test_debug", "[luajit]") {
    auto L = luaL_newstate();
    REQUIRE(L != nullptr);

    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
    lua_sethook(L, lua_hook, LUA_MASKCOUNT, 100);

    auto script = R"===(
        local i = 0
        while true do
            i = i + 1
            print(i)
        end
        print('i is ' .. i)
    )===";

    auto r = luaL_dostring(L, script);
    REQUIRE(r != 0);
}