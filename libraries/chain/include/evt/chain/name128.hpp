/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <string.h>
#include <iosfwd>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fc/reflect/reflect.hpp>
#include <fc/io/raw.hpp>

namespace jmzk { namespace chain {

using uint128_t = __uint128_t;

struct name128 {
public:
    uint128_t value = 0;

public:
    enum {
        i32 = 0, // <=  5 chars (2 +  5 * 6 =  32)
        i64,     // <= 10 chars (2 + 10 * 6 =  62)
        i96,     // <= 15 chars (2 + 15 * 6 =  92)
        i128     // <= 21 chars (2 + 21 * 6 = 128)
    };

public:
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
        constexpr auto flag = ((uint128_t)0x3f << 2);
        return !(value & flag);
    }

    constexpr name128() = default;
    constexpr name128(const name128&) = default;
    constexpr name128(const uint128_t& v) : value(v) {}

    name128(const char* str) { set(str); }
    name128(const std::string& str) { set(str); }

    void set(const char* str);
    void set(const std::string& str);

    static name128 from_number(uint64_t v);

    explicit operator std::string() const;
    explicit operator bool() const { return value; }
    explicit operator uint128_t() const { return value; }

    std::string
    to_string() const {
        return std::string(*this);
    }

    friend std::ostream&
    operator<<(std::ostream& out, const name128& n) {
        return out << std::string(n);
    }

    name128&
    operator=(const uint128_t& v) {
        value = v;
        return *this;
    }

    name128&
    operator=(const std::string& n) {
        value = name128(n).value;
        return *this;
    }

    name128&
    operator=(const char* n) {
        value = name128(n).value;
        return *this;
    }

    friend bool
    operator<(const name128& a, const name128& b) {
        return a.value < b.value;
    }

    friend bool
    operator<=(const name128& a, const name128& b) {
        return a.value <= b.value;
    }

    friend bool
    operator>(const name128& a, const name128& b) {
        return a.value > b.value;
    }

    friend bool
    operator>=(const name128& a, const name128& b) {
        return a.value >= b.value;
    }

    friend bool
    operator==(const name128& a, const name128& b) {
        return a.value == b.value;
    }

    friend bool
    operator==(const name128& a, uint128_t b) {
        return a.value == b;
    }
    
    friend bool
    operator!=(const name128& a, uint128_t b) {
        return a.value != b;
    }

    friend bool
    operator!=(const name128& a, const name128& b) {
        return a.value != b.value;
    }
};

static constexpr uint128_t
char_to_symbol128(char c) {
    if(c >= 'a' && c <= 'z') {
        return (c - 'a' + 12);
    }
    if(c >= 'A' && c <= 'Z') {
        return (c - 'A' + 38);
    }
    if(c >= '0' && c <= '9') {
        return (c - '0' + 2);
    }
    if(c == '-') {
        return 1;
    }
    return 0;
}

static constexpr uint128_t
string_to_name128(const char* str) {
    if(str == nullptr) {
        return uint128_t(0);
    }

    auto len = __builtin_strlen(str);
    if(len == 0) {
        return uint128_t(0);
    }

    auto name = uint128_t();
    for(auto i = 0u; i <= std::min(len, 20ul); ++i) {
        name |= (char_to_symbol128(str[i]) & 0x3f) << (2 + 6 * i);
    }
    if(len <= 5) {
        name |= name128::i32;
    }
    else if(len <= 10) {
        name |= name128::i64;
    }
    else if(len <= 15) {
        name |= name128::i96;
    }
    else {
        name |= name128::i128;
    }

    return name;
}

#define N128(X) jmzk::chain::string_to_name128(#X)

inline std::vector<name128>
sort_names(std::vector<name128>&& names) {
    fc::deduplicate(names);
    return std::move(names);
}

}}  // namespace jmzk::chain

namespace std {
using uint128_t = __uint128_t;

template <>
struct hash<jmzk::chain::name128> : private hash<uint128_t> {
    using argument_type = jmzk::chain::name128;

    std::size_t
    operator()(const argument_type& name) const noexcept {
        return hash<uint128_t>::operator()(name.value);
    }
};

}  // namespace std

namespace fc {

class variant;

void to_variant(const jmzk::chain::name128& name, fc::variant& v);
void from_variant(const fc::variant& v, jmzk::chain::name128& name);

namespace raw {

using jmzk::chain::name128;

template<>
struct packer<name128> {
    template<typename Stream>
    static void
    pack(Stream& out, const name128& n) {
        auto tag = (int)n.value & 0x03;
        switch(tag) {
        case name128::i32: {
            out.write((char*)&n, 4);
            break;
        }
        case name128::i64: {
            out.write((char*)&n, 8);
            break;
        }
        case name128::i96: {
            out.write((char*)&n, 12);
            break;
        }
        case name128::i128: {
            out.write((char*)&n, 16);
            break;
        }
        }  // switch
    }
};

template<>
struct unpacker<name128> {
    template<typename Stream>
    static void
    unpack(Stream& in, name128& n) {
        n.value = 0;
        in.read((char*)&n, 4);

        auto tag = (int)n.value & 0x03;
        switch(tag) {
        case name128::i32: {
            break;
        }
        case name128::i64: {
            in.read((char*)&n + 4, 4);
            break;
        }
        case name128::i96: {
            in.read((char*)&n + 4, 8);
            break;
        }
        case name128::i128: {
            in.read((char*)&n + 4, 12);
            break;
        }
        }  // switch
    }
};

}  // namespace raw

}  // namespace fc

namespace fmt {

template <>
struct formatter<jmzk::chain::name128> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const jmzk::chain::name128& n, FormatContext &ctx) {
        return format_to(ctx.begin(), n.to_string());
    }
};

}  // namespace fmt