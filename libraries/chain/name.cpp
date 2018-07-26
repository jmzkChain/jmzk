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
    value = string_to_name(str);
    EVT_ASSERT(to_string() == string(str), name_type_exception,
               "Name not properly normalized (name: ${name}, normalized: ${normalized}) ",
               ("name", string(str))("normalized", to_string()));
}

name::operator string() const {
    static const char* charmap = ".abcdefghijklmnopqrstuvwxyz12345";

    string str(13, '.');

    uint64_t tmp = value;
    for(uint32_t i = 0; i <= 12; ++i) {
        char c      = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
        str[12 - i] = c;
        tmp >>= (i == 0 ? 4 : 5);
    }

    boost::algorithm::trim_right_if(str, [](char c) { return c == '.'; });
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
