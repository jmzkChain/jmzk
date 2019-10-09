/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string.h>
#include <iosfwd>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fc/reflect/reflect.hpp>

namespace evt { namespace chain {
using std::string;

static constexpr uint64_t
char_to_symbol(char c) {
    if(c >= 'a' && c <= 'z')
        return (c - 'a') + 1;
    if(c >= '1' && c <= '5')
        return (c - '1') + 27;
    return 0;
}

static constexpr uint64_t
string_to_name(const char* str) {
    uint64_t name = 0;
    int      i    = 0;
    for(; str[i] && i < 12; ++i) {
        // NOTE: char_to_symbol() returns char type, and without this explicit
        // expansion to uint64 type, the compilation fails at the point of usage
        // of string_to_name(), where the usage requires constant (compile time) expression.
        name |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
    }

    // The for-loop encoded up to 60 high bits into uint64 'name' variable,
    // if (strlen(str) > 12) then encode str[12] into the low (remaining)
    // 4 bits of 'name'
    if(i == 12)
        name |= char_to_symbol(str[12]) & 0x0f;
    return name;
}

#define N(X) evt::chain::string_to_name(#X)

struct name {
    uint64_t value = 0;

    bool
    empty() const {
        return 0 == value;
    }

    bool
    good() const {
        return !empty();
    }

    bool
    reserved() const {
        constexpr auto flag = ((uint64_t)0x1f << (64-5));
        return !(value & flag);
    }

    name(const char* str) { set(str); }
    name(const string& str) { set(str.c_str()); }

    void set(const char* str);

    constexpr name(uint64_t v) : value(v) {}
    constexpr name() {}

    explicit operator string() const;
    explicit operator bool() const { return value; }
    explicit operator uint64_t() const { return value; }

    string
    to_string() const {
        return string(*this);
    }

    name&
    operator=(uint64_t v) {
        value = v;
        return *this;
    }

    name&
    operator=(const string& n) {
        value = name(n).value;
        return *this;
    }

    name&
    operator=(const char* n) {
        value = name(n).value;
        return *this;
    }

    friend std::ostream&
    operator<<(std::ostream& out, const name& n) {
        return out << string(n);
    }

    friend bool
    operator<(const name& a, const name& b) {
        return a.value < b.value;
    }

    friend bool
    operator<=(const name& a, const name& b) {
        return a.value <= b.value;
    }

    friend bool
    operator>(const name& a, const name& b) {
        return a.value > b.value;
    }

    friend bool
    operator>=(const name& a, const name& b) {
        return a.value >= b.value;
    }

    friend bool
    operator==(const name& a, const name& b) {
        return a.value == b.value;
    }

    friend bool
    operator==(const name& a, uint64_t b) {
        return a.value == b;
    }

    friend bool
    operator!=(const name& a, uint64_t b) {
        return a.value != b;
    }

    friend bool
    operator!=(const name& a, const name& b) {
        return a.value != b.value;
    }
};

inline std::vector<name>
sort_names(std::vector<name>&& names) {
    fc::deduplicate(names);
    return std::move(names);
}

}}  // namespace evt::chain

namespace std {
template <>
struct hash<evt::chain::name> : private hash<uint64_t> {
    using argument_type = evt::chain::name;

    std::size_t
    operator()(const argument_type& name) const noexcept {
        return hash<uint64_t>::operator()(name.value);
    }
};

};  // namespace std

namespace fc {

class variant;
void to_variant(const evt::chain::name& name, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::name& name);

}  // namespace fc

namespace fmt {

template <>
struct formatter<evt::chain::name> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const evt::chain::name& n, FormatContext &ctx) {
        return format_to(ctx.begin(), n.to_string());
    }
};

}  // namespace fmt

FC_REFLECT(evt::chain::name, (value));
