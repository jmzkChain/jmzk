/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/name128.hpp>
#include <boost/algorithm/string.hpp>
#include <evt/chain/exceptions.hpp>
#include <fc/variant.hpp>

namespace evt { namespace chain {

void
name128::set(const char* str) {
    const auto len = strnlen(str, 22);
    EVT_ASSERT(len <= 21, name128_type_exception, "Name128 is longer than 21 characters (${name}) ",
               ("name", string(str)));
    value = string_to_name128(str);
    EVT_ASSERT(to_string() == string(str), name128_type_exception,
               "Name128 not properly normalized (name: ${name}, normalized: ${normalized}) ",
               ("name", string(str))("normalized", to_string()));
}

name128::operator string() const {
    static const char* charmap = ".-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    auto str = string(21, '.');
    
    auto tmp = value;
    tmp >>= 2;
    for(auto i = 0u; i <= 20; ++i, tmp >>= 6) {
        auto c = charmap[tmp & 0x3f];
        str[i] = c;
    }

    str.erase(str.find_last_not_of('.') + 1);
    return str;
}

}}  // namespace evt::chain


namespace fc {

void
to_variant(const evt::chain::name128& name, fc::variant& v) {
    v = std::string(name);
}
void
from_variant(const fc::variant& v, evt::chain::name128& name) {
    name = v.get_string();
}

}  // namespace fc
