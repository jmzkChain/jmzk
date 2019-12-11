#include <catch/catch.hpp>

#include <lua.hpp>

extern "C" {

int __attribute__ ((visibility("default"))) __attribute__ ((noinline)) ladd(int a, int b) {
    return a + b;
}

}

static void lua_hook(lua_State* L, lua_Debug* ar) {
    static int count = 0;

    lua_getinfo(L, "nS", ar);
    printf("hook %d from %s: %s %s\n", ar->event, ar->what, ar->name, ar->namewhat);

    if(++count == 10) {
        luaL_error(L, "exceed hook count");
    }
}

TEST_CASE("test_debug", "[luajit]") {
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