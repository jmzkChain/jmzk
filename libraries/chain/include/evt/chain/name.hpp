/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fc/reflect/reflect.hpp>
#include <iosfwd>
#include <string>

namespace evt { namespace chain {
using std::string;
using uint128_t = __uint128_t;

static constexpr uint64_t
char_to_symbol(char c) {
    if(c >= 'a' && c <= 'z')
        return (c - 'a') + 6;
    if(c >= '1' && c <= '5')
        return (c - '1') + 1;
    return 0;
}

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

static constexpr uint128_t
string_to_name128(const char* str) {
    uint128_t name = 0;
    int       i    = 0;
    for(; str[i] && i < 20; ++i) {
        name |= (char_to_symbol128(str[i]) & 0x3f) << (128 - 6 * (i + 1));
    }
    if(i == 20) {
        name |= (char_to_symbol128(str[20]) & 0x3f);
    }
    return name;
}

#define N(X) evt::chain::string_to_name(#X)
#define N128(X) evt::chain::string_to_name128(#X)

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

    name(const char* str) { set(str); }
    name(const string& str) { set(str.c_str()); }

    void set(const char* str);

    template <typename T>
    name(T v)
        : value(v) {}
    name() {}

    explicit operator string() const;

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

    operator bool() const { return value; }
    operator uint64_t() const { return value; }
};

inline std::vector<name>
sort_names(std::vector<name>&& names) {
    fc::deduplicate(names);
    return names;
}

struct name128 {
    uint128_t value = 0;

    bool
    empty() const {
        return 0 == value;
    }

    bool
    good() const {
        return !empty();
    }

    name128(const char* str) { set(str); }
    name128(const string& str) { set(str.c_str()); }

    void set(const char* str);

    template <typename T>
    name128(T v)
        : value(v) {}
    name128() {}

    explicit operator string() const;

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

    friend std::ostream&
    operator<<(std::ostream& out, const name128& n) {
        return out << string(n);
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

    operator bool() const { return value; }
    operator uint128_t() const { return value; }
};

inline std::vector<name128>
sort_names(std::vector<name128>&& names) {
    fc::deduplicate(names);
    return names;
}

}}  // namespace evt::chain

namespace std {
template <>
struct hash<evt::chain::name> : private hash<uint64_t> {
    typedef evt::chain::name                     argument_type;
    typedef typename hash<uint64_t>::result_type result_type;
    result_type
    operator()(const argument_type& name) const noexcept {
        return hash<uint64_t>::operator()(name.value);
    }
};

template <>
struct hash<evt::chain::name128> : private hash<uint64_t> {
    typedef evt::chain::name128                  argument_type;
    typedef typename hash<uint64_t>::result_type result_type;
    result_type
    operator()(const argument_type& name) const noexcept {
        return hash<uint64_t>::operator()(name.value);
    }
};
};  // namespace std

namespace fc {
class variant;
void to_variant(const evt::chain::name& c, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::name& check);
void to_variant(const evt::chain::name128& c, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::name128& check);
}  // namespace fc

FC_REFLECT(evt::chain::name, (value))
FC_REFLECT(evt::chain::name128, (value))
