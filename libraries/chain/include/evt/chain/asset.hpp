/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/symbol.hpp>

/// 'EVT' with 5 digits of precision
#define EVT_SYMBOL_VALUE  (int64_t(5) | (uint64_t('E') << 8) | (uint64_t('V') << 16) | (uint64_t('T') << 24))
static const evt::chain::symbol EVT_SYMBOL(EVT_SYMBOL_VALUE);

/// Defined to be largest power of 10 that fits in 53 bits of precision
#define EVT_MAX_SHARE_SUPPLY   int64_t(1'000'000'000'000'000ll)

namespace evt { namespace chain {

/**

asset includes amount and currency symbol

asset::from_string takes a string of the form "10.0000 CUR" and constructs an asset 
with amount = 10 and symbol(4,"CUR")

*/
struct asset {
    static constexpr int64_t max_amount = (1LL << 62) - 1;

    explicit asset(share_type a = 0, symbol id = EVT_SYMBOL)
        : amount(a)
        , sym(id) {
        EVT_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62");
        EVT_ASSERT(sym.valid(), asset_type_exception, "invalid symbol");
    }

    bool
    is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

    bool
    is_valid() const { return is_amount_within_range() && sym.valid(); }

    double
    to_real() const { return static_cast<double>(amount) / precision(); }

    uint8_t decimals() const;
    string  symbol_name() const;
    int64_t precision() const;

    const symbol&
    get_symbol() const { return sym; }

    share_type
    get_amount() const { return amount; }

    static asset from_string(const string& from);
    string       to_string() const;

    explicit operator string() const {
        return to_string();
    }

    asset&
    operator+=(const asset& o) {
        FC_ASSERT(get_symbol() == o.get_symbol());
        amount += o.amount;
        return *this;
    }

    asset&
    operator-=(const asset& o) {
        FC_ASSERT(get_symbol() == o.get_symbol());
        amount -= o.amount;
        return *this;
    }
    asset
    operator-() const { return asset(-amount, get_symbol()); }

    friend bool
    operator==(const asset& a, const asset& b) {
        return std::tie(a.get_symbol(), a.amount) == std::tie(b.get_symbol(), b.amount);
    }

    friend bool
    operator<(const asset& a, const asset& b) {
        FC_ASSERT(a.get_symbol() == b.get_symbol());
        return std::tie(a.amount, a.get_symbol()) < std::tie(b.amount, b.get_symbol());
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
        FC_ASSERT(a.get_symbol() == b.get_symbol());
        return asset(a.amount - b.amount, a.get_symbol());
    }

    friend asset
    operator+(const asset& a, const asset& b) {
        FC_ASSERT(a.get_symbol() == b.get_symbol());
        return asset(a.amount + b.amount, a.get_symbol());
    }

    friend std::ostream&
    operator<<(std::ostream& out, const asset& a) { return out << a.to_string(); }
    
    friend struct fc::reflector<asset>;

    void
    reflector_verify() const {
        EVT_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62");
        EVT_ASSERT(sym.valid(), asset_type_exception, "invalid symbol");
    }

private:
    share_type amount;
    symbol     sym;
};

struct extended_asset {
    extended_asset() {}
    extended_asset(asset a, name n)
        : quantity(a)
        , contract(n) {}
    asset quantity;
    name  contract;
};

bool operator<(const asset& a, const asset& b);
bool operator<=(const asset& a, const asset& b);

}}  // namespace evt::chain

namespace fc {
inline void
to_variant(const evt::chain::asset& var, fc::variant& vo) {
    vo = var.to_string();
}
inline void
from_variant(const fc::variant& var, evt::chain::asset& vo) {
    vo = evt::chain::asset::from_string(var.get_string());
}
}  // namespace fc

FC_REFLECT(evt::chain::asset, (amount)(sym))
FC_REFLECT(evt::chain::extended_asset, (quantity)(contract))
