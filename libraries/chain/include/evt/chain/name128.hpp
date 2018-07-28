/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <iosfwd>
#include <string>

namespace evt { namespace chain {
using std::string;
using uint128_t = __uint128_t;

struct name128 {
public:
    uint128_t value = 0;

public:
    enum {
        i8 = 0,  // <=  0 chars (8 +  0     =   8)
        i32,     // <=  5 chars (2 +  5 * 6 =  32)
        i64,     // <= 10 chars (2 + 10 * 6 =  62)
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

    constexpr name128() = default;
    constexpr name128(uint128_t v) : value(v) {}

    name128(const char* str) { set(str); }
    name128(const string& str) { set(str.c_str()); }

    void set(const char* str);

    explicit operator string() const;
    operator bool() const { return value; }
    operator uint128_t() const { return value; }

    string
    to_string() const {
        return string(*this);
    }

    name128&
    operator=(uint128_t v) {
        value = v;
        return *this;
    }

    name128&
    operator=(const string& n) {
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

    template<typename Stream>
    friend Stream&
    operator<<(Stream& out, const name128& n) {
        auto tag = (int)n.value & 0x03;
        switch(tag) {
        case name128::i8: {
            out.write((char*)&n, 1);
            break;
        }
        case name128::i32: {
            out.write((char*)&n, 4);
            break;
        }
        case name128::i64: {
            out.write((char*)&n, 8);
            break;
        }
        case name128::i128: {
            out.write((char*)&n, 16);
            break;
        }
        }  // switch

        return out;
    }

    template<typename Stream>
    friend Stream&
    operator>>(Stream& in, name128& n) {
        in.read((char*)&n, 1);

        auto tag = (int)n.value & 0x03;
        switch(tag) {
        case name128::i8: {
            break;
        }
        case name128::i32: {
            in.read((char*)&n + 1, 3);
            break;
        }
        case name128::i64: {
            in.read((char*)&n + 1, 7);
            break;
        }
        case name128::i128: {
            in.read((char*)&n + 1, 15);
            break;
        }
        }  // switch

        return in;
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
    auto len = strlen(str);
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
    else {
        name |= name128::i128;
    }

    return name;
}

#define N128(X) evt::chain::string_to_name128(#X)

inline std::vector<name128>
sort_names(std::vector<name128>&& names) {
    fc::deduplicate(names);
    return names;
}

}}  // namespace evt::chain

namespace std {
using uint128_t = __uint128_t;

template <>
struct hash<evt::chain::name128> : private hash<uint128_t> {
    using argument_type = evt::chain::name128;
    using result_type   = typename hash<uint128_t>::result_type;

    result_type
    operator()(const argument_type& name) const noexcept {
        return hash<uint128_t>::operator()(name.value);
    }
};

};  // namespace std

namespace fc {

class variant;

void to_variant(const evt::chain::name128& name, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::name128& name);

}  // namespace fc
