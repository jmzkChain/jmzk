/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/name128.hpp>
#include <jmzk/chain/exceptions.hpp>
#include <fc/variant.hpp>

namespace jmzk { namespace chain {

void
name128::set(const char* str) {
    const auto len = strnlen(str, 22);
    jmzk_ASSERT(len <= 21, name128_type_exception, "Name128 is longer than 21 characters (${name}) ",
               ("name", std::string(str)));
    jmzk_ASSERT(len > 0, name128_type_exception, "Name128 cannot be empty");
    value = string_to_name128(str);
    jmzk_ASSERT(to_string() == std::string(str), name128_type_exception,
               "Name128 not properly normalized (name: ${name}, normalized: ${normalized}) ",
               ("name", std::string(str))("normalized", to_string()));
}

void
name128::set(const std::string& str) {
    const auto len = str.size();
    jmzk_ASSERT(len <= 21, name128_type_exception, "Name128 is longer than 21 characters (${name}) ",
               ("name", str));
    value = string_to_name128(str.c_str());
    jmzk_ASSERT(to_string() == str, name128_type_exception,
               "Name128 not properly normalized (name: ${name}, normalized: ${normalized}) ",
               ("name", str)("normalized", to_string()));
}

name128::operator std::string() const {
    static const char* charmap = ".-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    auto str = std::string(21, '.');
    
    auto stop = 0u;
    auto tmp  = value >> 2;
    auto tag  = (int)value & 0x03;

    switch(tag) {
    case i32: {
        stop = 5;
        break;
    }
    case i64: {
        stop = 10;
        break;
    }
    case i96: {
        stop = 15;
        break;
    }
    case i128: {
        stop = 21;
        break;
    }
    }  // switch

    for(auto i = 0u; i < stop; ++i, tmp >>= 6) {
        auto c = charmap[tmp & 0x3f];
        str[i] = c;
    }

    str.erase(str.find_last_not_of('.', stop) + 1);
    return str;
}

name128
name128::from_number(uint64_t v) {
    auto r = name128();

    int i = 0;
    do {
        auto d = v % 10;
        r.value |= (uint128_t(d + 2) & 0x3f) << (128 - (6 * (i++ + 1)));

        v /= 10;
    }
    while(v > 0);

    r.value >>= 6 * (21 - i);

    if(i <= 5) {
        r.value |= name128::i32;
    }
    else if(i <= 10) {
        r.value |= name128::i64;
    }
    else if(i <= 15) {
        r.value |= name128::i96;
    }
    else {
        r.value |= name128::i128;
    }

    return r;
}

}}  // namespace jmzk::chain


namespace fc {

void
to_variant(const jmzk::chain::name128& name, fc::variant& v) {
    v = std::string(name);
}
void
from_variant(const fc::variant& v, jmzk::chain::name128& name) {
    name = v.get_string();
}

}  // namespace fc
