#include <evt/chain/contracts/lua_json.hpp>

#include <stack>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

namespace internal {

using namespace rapidjson;

class LuaHandler : public BaseReaderHandler<UTF8<>, LuaHandler> {
public:
    LuaHandler(lua_State* L, bool is_args) : L_(L), is_args_(is_args)
    { }

public:
    bool Null();
    bool Bool(bool b);
    bool Int(int i);
    bool Uint(unsigned int i);
    bool Int64(int64_t int64);
    bool Uint64(uint64_t uint64);
    bool Double(double d);
    bool RawNumber(const Ch* str, SizeType len, bool copy);
    bool String(const Ch* ch, SizeType len, bool copy);
    bool StartObject();
    bool Key(const Ch* str, SizeType len, bool copy);
    bool EndObject(SizeType);
    bool StartArray();
    bool EndArray(SizeType);

private:
    int TableInsert();

private:
    enum Type {
        kObject,
        kArray
    };
    std::stack<Type> types_;
    lua_State*  L_;
    bool is_args_;
};

int
LuaHandler::TableInsert() {
    if(types_.empty()) {
#ifndef NDEBUG
        auto top = lua_gettop(L_);
        assert(top == 1);
#endif
        return LUA_OK;
    }
    auto type = types_.top();
    switch (type) {
    case kObject: {
        assert(lua_type(L_, -2) == LUA_TSTRING);
        assert(lua_type(L_, -3) == LUA_TTABLE);
        lua_rawset(L_, -3);
        break;
    }
    case kArray: {
        assert(lua_type(L_, -2) == LUA_TTABLE);
        auto len = (int)lua_objlen(L_, -2);
        lua_rawseti(L_, -2, len + 1);
        break;
    }
    } // switch
    return LUA_OK;
}

bool
LuaHandler::StartObject() {
    lua_newtable(L_);
    types_.push(kObject);
    return true;
}

bool
LuaHandler::EndObject(SizeType) {
    if(types_.empty()) {
        return false;
    }
    if(types_.top() != kObject) {
        return false;
    }
    types_.pop();
    TableInsert();
    return true;
}

bool
LuaHandler::StartArray() {
    lua_newtable(L_);
    types_.push(kArray);
    return true;
}

bool
LuaHandler::EndArray(SizeType len) {
    if(types_.empty()) {
        return false;
    }
    if(types_.top() != kArray) {
        return false;
    }
    types_.pop();
    TableInsert();

    if(types_.size() == 0 && is_args_) {
        // end root array
        lua_pushstring(L_, "n");
        lua_pushinteger(L_, len);
        lua_rawset(L_, -3);
    }
    return true;
}

bool
LuaHandler::Key(const char* str, SizeType len, bool copy) {
    if(types_.top() != kObject) {
        return false;
    }
    lua_pushlstring(L_, str, len);
    return true;
}

bool
LuaHandler::Null() {
    lua_pushnil(L_);
    TableInsert();
    return true;
}

bool
LuaHandler::Bool(bool b) {
    lua_pushboolean(L_, b);
    TableInsert();
    return true;
}

bool
LuaHandler::Int(int i) {
    lua_pushinteger(L_, i);
    TableInsert();
    return true;
}

bool
LuaHandler::Uint(unsigned int i) {
    lua_pushinteger(L_, i);
    TableInsert();
    return true;
}

bool
LuaHandler::Int64(int64_t i) {
    lua_pushinteger(L_, i);
    TableInsert();
    return true;
}

bool
LuaHandler::Uint64(uint64_t i) {
    lua_pushinteger(L_, i);
    TableInsert();
    return true;
}

bool
LuaHandler::Double(double d) {
    lua_pushnumber(L_, d);
    TableInsert();
    return true;
}

bool
LuaHandler::RawNumber(const char* str, SizeType len, bool copy) {
    assert(false);
    return false;
}

bool
LuaHandler::String(const char* ch, SizeType len, bool copy) {
    lua_pushlstring(L_, ch, len);
    TableInsert();
    return true;
}

enum PackError {
    kRootNotTable = -1,
    kKeyNotString = -2,
    kValueTypeNotValid = -3,
    kNotValidArray = -4
};

template <template<typename> typename TWriterBase, typename TWriter = TWriterBase<StringBuffer>>
class LuaPacker {
public:
    LuaPacker(lua_State* L, StringBuffer& sb, bool is_args) : L_(L),
                                                              writer_(sb),
                                                              is_args_(is_args)
    { }

public:
    int
    Pack() {
        if(lua_type(L_, -1) != LUA_TTABLE) {
            err_ = kRootNotTable;
            return KYOTO_FAIL;
        }
        auto r = PackTable(true);
        if(!r) {
            return KYOTO_FAIL;
        }
        assert(writer_.IsComplete());
        return KYOTO_OK;
    }

    int GetError() { return err_; }

private:
    int
    PackTable(bool is_root = false) {
        assert(lua_type(L_, -1) == LUA_TTABLE);
        if(is_root && is_args_) {
            // special pack the args table
            return PackArray(true);
        }

        lua_rawgeti(L_, -1, 1);
        if(lua_type(L_, -1) != LUA_TNIL) {
            lua_pop(L_, 1);
            return PackArray();
        }
        else {
            lua_pop(L_, 1);
            return PackMap();
        }
    }

    int
    PackMap() {
        writer_.StartObject();
        lua_pushnil(L_);
        while(lua_next(L_, -2) != 0) {
            auto tv = lua_type(L_, -1);
            auto tk = lua_type(L_, -2);
            if(tk != LUA_TSTRING) {
                err_ = kKeyNotString;
                return KYOTO_FAIL;
            }
            writer_.Key(lua_tostring(L_, -2));
            auto r = 0;
            switch(tv) {
            case LUA_TNIL: {
                r = PackNil();
                break;
            }
            case LUA_TBOOLEAN: {
                r = PackBool();
                break;
            }
            case LUA_TNUMBER: {
                r = PackNumber();
                break;
            }
            case LUA_TSTRING: {
                r = PackString();
                break;
            }
            case LUA_TTABLE: {
                r = PackTable();
                break;
            }
            default: {
                err_ = kValueTypeNotValid;
                return KYOTO_FAIL;
            }
            } // switch
            if(!r) {
                return KYOTO_FAIL;
            }
        }
        writer_.EndObject();
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

    int
    PackArray(bool is_root = false) {
#ifndef NDEBUG
        auto top = lua_gettop(L_);
#endif
        writer_.StartArray();

        size_t n = 0;
        if(is_root && is_args_) {
            lua_pushstring(L_, "n");
            lua_rawget(L_, -2);
            if(lua_type(L_, -1) != LUA_TNUMBER) {
                return KYOTO_FAIL;
            }
            n = (size_t)lua_tonumber(L_, -1);
            lua_pop(L_, 1);
        }
        else {
            n = lua_objlen(L_, -1);
        }

        for(size_t i = 1; i <= n ; i++) {
            lua_rawgeti(L_, -1, i);
            auto tv = lua_type(L_, -1);
            auto r = 0;
            switch(tv) {
            case LUA_TNIL: {
                r = PackNil();
                break;
            }
            case LUA_TBOOLEAN: {
                r = PackBool();
                break;
            }
            case LUA_TNUMBER: {
                r = PackNumber();
                break;
            }
            case LUA_TSTRING: {
                r = PackString();
                break;
            }
            case LUA_TTABLE: {
                r = PackTable();
                break;
            }
            default: {
                err_ = kValueTypeNotValid;
                return KYOTO_FAIL;
            }
            } // switch
            if(!r) {
                return KYOTO_FAIL;
            }
        }
        writer_.EndArray();
#ifndef NDEBUG
        auto top2 = lua_gettop(L_);
        assert(top == top2);
#endif
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

    int
    PackString() {
        size_t sz;
        auto s = lua_tolstring(L_, -1, &sz);
        if(s == nullptr) {
            return KYOTO_FAIL;
        }
        writer_.String(s, sz, true);
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

    int
    PackNumber() {
        auto n = lua_tonumber(L_, -1);
        writer_.Double(n);
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

    int
    PackBool() {
        auto b = lua_toboolean(L_, -1);
        writer_.Bool(b);
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

    int
    PackNil() {
        writer_.Null();
        lua_pop(L_, 1);
        return KYOTO_OK;
    }

private:
    lua_State*  L_;
    TWriter     writer_;
    PackError   err_;
    bool        is_args_;
};

} // namespace internal

extern "C" {

static int
ldeserialize(lua_State* L) {
    using namespace rapidjson;
    using namespace internal;

    bool args = false;
    if(lua_gettop(L) > 1) {
        args = lua_toboolean(L, -1) == 1;
        lua_settop(L, 1);
    }

    auto json = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    Reader reader;
    LuaHandler handler(L, args);
    StringStream ss(json);
    if(!reader.Parse(ss, handler)) {
        auto e = reader.GetParseErrorCode();
        return luaL_error(L, "parse json failed: %s at %lx. content:\n %s",
            GetParseError_En(e), reader.GetErrorOffset(), json);
    }
    assert(lua_type(L, -1) == LUA_TTABLE);
    return 1;
}

// 1: table to be packed
// 2: is pretty print
// 3: is packed
static int
lserialize(lua_State* L) {
    bool pretty = false, args = false;
    if(lua_gettop(L) > 1) {
        pretty = lua_toboolean(L, -2) == 1;
    }
    if(lua_gettop(L) > 2) {
        args = lua_toboolean(L, -1) == 1;
    }
    lua_settop(L, 1);

    using namespace rapidjson;
    using namespace internal;

    StringBuffer sb;
    auto r = 0;
    auto err = 0;
    if(!pretty) {
        LuaPacker<Writer> packer(L, sb, args);
        r = packer.Pack();
        err = packer.GetError();
    }
    else {
        LuaPacker<PrettyWriter> packer(L, sb, args);
        r = packer.Pack();
        err = packer.GetError();
    }

    if(!r) {
        switch(err) {
        case kRootNotTable: {
            return luaL_error(L, "toot must be table");
        }
        case kKeyNotString: {
            return luaL_error(L, "key need be of string type");
        }
        case kValueTypeNotValid: {
            return luaL_error(L, "value must be one of map, array, string, number or bool");
        }
        case kNotValidArray: {
            return luaL_error(L, "table is not a valid array (elements are not continuous)");
        }
        default: {
            return luaL_error(L, "unknown error");
        }
        } // switch
    }
    lua_pushlstring(L, sb.GetString(), sb.GetSize());
    return 1;
}

int
luaopen_json(lua_State* L) {
    luaL_Reg l[] = {
        { "deserialize", ldeserialize },
        { "serialize", lserialize },
        { NULL, NULL}
    };
    luaL_newlib(L, l);
    return 1;
}

}  // extern "C"
