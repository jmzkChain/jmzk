#include <evt/chain/contracts/lua_json.hpp>

#include <stack>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

namespace internal {

using namespace rapidjson;

class lua_handler : public BaseReaderHandler<UTF8<>, lua_handler> {
public:
    lua_handler(lua_State* L, bool is_args) : L_(L), is_args_(is_args) { }

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
    int table_append();

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
lua_handler::table_append() {
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
lua_handler::StartObject() {
    lua_newtable(L_);
    types_.push(kObject);
    return true;
}

bool
lua_handler::EndObject(SizeType) {
    if(types_.empty()) {
        return false;
    }
    if(types_.top() != kObject) {
        return false;
    }
    types_.pop();
    table_append();
    return true;
}

bool
lua_handler::StartArray() {
    lua_newtable(L_);
    types_.push(kArray);
    return true;
}

bool
lua_handler::EndArray(SizeType len) {
    if(types_.empty()) {
        return false;
    }
    if(types_.top() != kArray) {
        return false;
    }
    types_.pop();
    table_append();

    if(types_.size() == 0 && is_args_) {
        // end root array
        lua_pushstring(L_, "n");
        lua_pushinteger(L_, len);
        lua_rawset(L_, -3);
    }
    return true;
}

bool
lua_handler::Key(const char* str, SizeType len, bool copy) {
    if(types_.top() != kObject) {
        return false;
    }
    lua_pushlstring(L_, str, len);
    return true;
}

bool
lua_handler::Null() {
    lua_pushnil(L_);
    table_append();
    return true;
}

bool
lua_handler::Bool(bool b) {
    lua_pushboolean(L_, b);
    table_append();
    return true;
}

bool
lua_handler::Int(int i) {
    lua_pushinteger(L_, i);
    table_append();
    return true;
}

bool
lua_handler::Uint(unsigned int i) {
    lua_pushinteger(L_, i);
    table_append();
    return true;
}

bool
lua_handler::Int64(int64_t i) {
    lua_pushinteger(L_, i);
    table_append();
    return true;
}

bool
lua_handler::Uint64(uint64_t i) {
    lua_pushinteger(L_, i);
    table_append();
    return true;
}

bool
lua_handler::Double(double d) {
    lua_pushnumber(L_, d);
    table_append();
    return true;
}

bool
lua_handler::RawNumber(const char* str, SizeType len, bool copy) {
    assert(false);
    return false;
}

bool
lua_handler::String(const char* ch, SizeType len, bool copy) {
    lua_pushlstring(L_, ch, len);
    table_append();
    return true;
}

enum PackError {
    kRootNotTable = -1,
    kKeyNotString = -2,
    kValueTypeNotValid = -3,
    kNotValidArray = -4
};

template <typename TWriter>
class lua_packer {
public:
    lua_packer(lua_State* L, StringBuffer& sb, bool is_args) : L_(L),
                                                               writer_(sb),
                                                               is_args_(is_args)
    { }

public:
    int
    pack() {
        if(lua_type(L_, -1) != LUA_TTABLE) {
            err_ = kRootNotTable;
            return 0;
        }
        auto r = pack_table(true);
        if(!r) {
            return 0;
        }
        assert(writer_.IsComplete());
        return 1;
    }

    int GetError() { return err_; }

private:
    int
    pack_table(bool is_root = false) {
        assert(lua_type(L_, -1) == LUA_TTABLE);
        if(is_root && is_args_) {
            // special pack the args table
            return pack_array(true);
        }

        lua_rawgeti(L_, -1, 1);
        if(lua_type(L_, -1) != LUA_TNIL) {
            lua_pop(L_, 1);
            return pack_array();
        }
        else {
            lua_pop(L_, 1);
            return pack_map();
        }
    }

    int
    pack_map() {
        writer_.StartObject();
        lua_pushnil(L_);
        while(lua_next(L_, -2) != 0) {
            auto tv = lua_type(L_, -1);
            auto tk = lua_type(L_, -2);
            if(tk != LUA_TSTRING) {
                err_ = kKeyNotString;
                return 0;
            }
            writer_.Key(lua_tostring(L_, -2));
            auto r = 0;
            switch(tv) {
            case LUA_TNIL: {
                r = pack_nil();
                break;
            }
            case LUA_TBOOLEAN: {
                r = pack_bool();
                break;
            }
            case LUA_TNUMBER: {
                r = pack_number();
                break;
            }
            case LUA_TSTRING: {
                r = pack_string();
                break;
            }
            case LUA_TTABLE: {
                r = pack_table();
                break;
            }
            default: {
                err_ = kValueTypeNotValid;
                return 0;
            }
            } // switch
            if(!r) {
                return 0;
            }
        }
        writer_.EndObject();
        lua_pop(L_, 1);
        return 1;
    }

    int
    pack_array(bool is_root = false) {
#ifndef NDEBUG
        auto top = lua_gettop(L_);
#endif
        writer_.StartArray();

        size_t n = 0;
        if(is_root && is_args_) {
            lua_pushstring(L_, "n");
            lua_rawget(L_, -2);
            if(lua_type(L_, -1) != LUA_TNUMBER) {
                return 0;
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
                r = pack_nil();
                break;
            }
            case LUA_TBOOLEAN: {
                r = pack_bool();
                break;
            }
            case LUA_TNUMBER: {
                r = pack_number();
                break;
            }
            case LUA_TSTRING: {
                r = pack_string();
                break;
            }
            case LUA_TTABLE: {
                r = pack_table();
                break;
            }
            default: {
                err_ = kValueTypeNotValid;
                return 0;
            }
            } // switch
            if(!r) {
                return 0;
            }
        }
        writer_.EndArray();
#ifndef NDEBUG
        auto top2 = lua_gettop(L_);
        assert(top == top2);
#endif
        lua_pop(L_, 1);
        return 1;
    }

    int
    pack_string() {
        size_t sz;
        auto s = lua_tolstring(L_, -1, &sz);
        if(s == nullptr) {
            return 0;
        }
        writer_.String(s, sz, true);
        lua_pop(L_, 1);
        return 1;
    }

    int
    pack_number() {
        auto n = lua_tonumber(L_, -1);
        writer_.Double(n);
        lua_pop(L_, 1);
        return 1;
    }

    int
    pack_bool() {
        auto b = lua_toboolean(L_, -1);
        writer_.Bool(b);
        lua_pop(L_, 1);
        return 1;
    }

    int
    pack_nil() {
        writer_.Null();
        lua_pop(L_, 1);
        return 1;
    }

private:
    lua_State* L_;
    TWriter    writer_;
    PackError  err_;
    bool       is_args_;
};

} // namespace internal

extern "C" {

static int
ldeserialize(lua_State* L) {
    using namespace rapidjson;
    using namespace ::internal;

    bool args = false;
    if(lua_gettop(L) > 1) {
        args = lua_toboolean(L, -1) == 1;
        lua_settop(L, 1);
    }

    auto json = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    Reader reader;
    lua_handler handler(L, args);
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
    using namespace ::internal;

    StringBuffer sb;
    auto r = 0;
    auto err = 0;
    if(!pretty) {
        lua_packer<Writer<StringBuffer>> packer(L, sb, args);
        r = packer.pack();
        err = packer.GetError();
    }
    else {
        lua_packer<PrettyWriter<StringBuffer>> packer(L, sb, args);
        r = packer.pack();
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
        { NULL, NULL }
    };
    luaL_newlib(L, l);
    return 1;
}

}  // extern "C"
