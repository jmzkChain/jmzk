/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <boost/algorithm/string.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/name.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

namespace evt { namespace chain {

void
name::set(const char* str) {
    const auto len = strnlen(str, 14);
    EVT_ASSERT(len <= 13, name_type_exception, "Name is longer than 13 characters (${name}) ", ("name", string(str)));
    EVT_ASSERT(len > 0, name_type_exception, "Name cannot be empty");
    value = string_to_name(str);
    EVT_ASSERT(to_string() == string(str), name_type_exception,
               "Name not properly normalized (name: ${name}, normalized: ${normalized}) ",
               ("name", string(str))("normalized", to_string()));
}

name::operator string() const {
    static const char* charmap = ".abcdefghijklmnopqrstuvwxyz12345";

    string str(13, '.');

    auto tmp = value;
    str[12] = charmap[tmp & 0x0f];
    tmp >>= 4;

    for(auto i = 1u; i <= 12; ++i, tmp >>= 5) {
        char c    = charmap[tmp & 0x1f];
        str[12-i] = c;
    }

    str.erase(str.find_last_not_of('.') + 1);
    return str;
}

}}  // namespace evt::chain

namespace fc {

void
to_variant(const evt::chain::name& name, fc::variant& v) {
    v = std::string(name);
}

void
from_variant(const fc::variant& v, evt::chain::name& name) {
    name = v.get_string();
}

}  // namespace fc
