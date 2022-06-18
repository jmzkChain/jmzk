/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <fmt/format.h>
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

#define EMPTY_SYM_ID 0
#define jmzk_SYM_ID   1
#define Pjmzk_SYM_ID  2

class symbol : public fc::reflect_init {
private:
    static constexpr uint8_t max_precision = 18;

public:
    symbol() = default;

    constexpr symbol(uint8_t p, uint32_t id)
        : value_(0) {
        jmzk_ASSERT(p <= max_precision, symbol_type_exception, "Exceed max precision");
        value_  = ((uint64_t)p << 32);
        value_ |= id;
    }

public:
    uint8_t  precision() const { return (uint8_t)(value_ >> 32); }
    uint32_t id() const { return (uint32_t)value_; }

    bool
    valid() const {
        return value_ == symbol(precision(), id()).value_;
    }

public:
    static symbol from_string(const string& from);
    string to_string() const;

    explicit operator string() const {
        return to_string();
    }

public:
    friend inline bool
    operator==(const symbol& lhs, const symbol& rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend inline bool
    operator!=(const symbol& lhs, const symbol& rhs) {
        return !(lhs == rhs);
    }

    friend std::ostream&
    operator<<(std::ostream& out, const symbol& s) { return out << s.to_string(); }

private:
    uint64_t value_;

public:
    friend struct fc::reflector<symbol>;

    void
    reflector_init() const {
        jmzk_ASSERT(valid(), symbol_type_exception, "invalid symbol");
    }
};

static constexpr symbol
jmzk_sym() {
    return symbol(5, jmzk_SYM_ID);
}

static constexpr symbol
pjmzk_sym() {
    return symbol(5, Pjmzk_SYM_ID);
}

static constexpr symbol
nav_sym() {  // net asset value
    return symbol(12, EMPTY_SYM_ID);
}

/**

asset includes amount and currency symbol

asset::from_string takes a string of the form "10.0000 CUR" and constructs an asset 
with amount = 10 and symbol(4,"CUR")

*/
struct asset : public fc::reflect_init {
public:
    struct integer{};
    static constexpr int64_t max_amount = (1LL << 62) - 1;

public:
    asset() = default;

    asset(share_type a, symbol sym)
        : amount_(a)
        , sym_(sym) {
        jmzk_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62");
        jmzk_ASSERT(sym_.valid(), asset_type_exception, "invalid symbol");
    }

public:
    bool is_amount_within_range() const { return -max_amount <= amount_ && amount_ <= max_amount; }
    bool is_valid() const { return is_amount_within_range() && sym_.valid(); }

    real_type to_real()   const { return real_type(amount_) / boost::multiprecision::pow(real_type(10), precision()); }
    double    to_double() const { return (double)amount_ / std::pow(10, precision()); }

    uint32_t symbol_id() const { return sym_.id(); };
    uint8_t  precision() const { return sym_.precision(); };

    symbol     sym() const { return sym_; }
    share_type amount() const { return amount_; }

public:
    static asset from_string(const string& from);
    static asset from_integer(share_type a, symbol s);
    string       to_string() const;

    explicit operator string() const {
        return to_string();
    }

    asset&
    operator+=(const asset& o) {
        jmzk_ASSERT(sym() == o.sym(), asset_type_exception, "addition between two different asset is not allowed");
        amount_ += o.amount();
        return *this;
    }

    asset&
    operator-=(const asset& o) {
        jmzk_ASSERT(sym() == o.sym(), asset_type_exception, "addition between two different asset is not allowed");
        amount_ -= o.amount();
        return *this;
    }

    asset operator-() const { return asset(-amount(), sym()); }

    friend bool
    operator==(const asset& a, const asset& b) {
        return a.sym() == b.sym() && a.amount() == b.amount();
    }

    friend bool
    operator<(const asset& a, const asset& b) {
        jmzk_ASSERT(a.sym() == b.sym(), asset_type_exception, "addition between two different asset is not allowed");
        return a.amount() < b.amount();
    }

    friend bool
    operator<=(const asset& a, const asset& b) { return (a == b) || (a < b); }

    friend bool
    operator!=(const asset& a, const asset& b) { return !(a == b); }

    friend bool
    operator>(const asset& a, const asset& b) { return !(a <= b); }

    friend bool
    operator>=(const asset& a, const asset& b) { return !(a < b); }

    friend asset
    operator-(const asset& a, const asset& b) {
        jmzk_ASSERT(a.sym() == b.sym(), asset_type_exception, "addition between two different asset is not allowed");
        return asset(a.amount() - b.amount(), a.sym());
    }

    friend asset
    operator+(const asset& a, const asset& b) {
        jmzk_ASSERT(a.sym() == b.sym(), asset_type_exception, "addition between two different asset is not allowed");
        return asset(a.amount() + b.amount(), a.sym());
    }

    friend std::ostream&
    operator<<(std::ostream& out, const asset& a) { return out << a.to_string(); }
    
public:
    friend struct fc::reflector<asset>;

    void
    reflector_init() const {
        jmzk_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62");
        jmzk_ASSERT(sym_.valid(), asset_type_exception, "invalid symbol");
    }

private:
    share_type amount_;
    symbol     sym_;
};

}}  // namespace jmzk::chain

namespace fc {

inline void
to_variant(const jmzk::chain::symbol& var, fc::variant& vo) {
    vo = var.to_string();
}

inline void
from_variant(const fc::variant& var, jmzk::chain::symbol& vo) {
    vo = jmzk::chain::symbol::from_string(var.get_string());
}

inline void
to_variant(const jmzk::chain::asset& var, fc::variant& vo) {
    vo = var.to_string();
}

inline void
from_variant(const fc::variant& var, jmzk::chain::asset& vo) {
    vo = jmzk::chain::asset::from_string(var.get_string());
}

}  // namespace fc

namespace fmt {

template <>
struct formatter<jmzk::chain::symbol> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const jmzk::chain::symbol& s, FormatContext &ctx) {
        return format_to(ctx.begin(), s.to_string());
    }
};

template <>
struct formatter<jmzk::chain::asset> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const jmzk::chain::asset& a, FormatContext &ctx) {
        return format_to(ctx.begin(), a.to_string());
    }
};

}  // namespace fmt

FC_REFLECT(jmzk::chain::symbol, (value_));
FC_REFLECT(jmzk::chain::asset, (amount_)(sym_));
