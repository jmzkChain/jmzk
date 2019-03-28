/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fmt/format.h>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

class percent_slim : public fc::reflect_init {
public:
    static constexpr uint32_t max_amount    = 100'000;
    static constexpr uint32_t max_precision = 5;

public:
    percent_slim() = default;

    percent_slim(uint32_t v)
        : v_(v) {
        EVT_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of percent_slim value must be less than 10^5");
    }

public:
    bool is_amount_within_range() const { return v_.value <= max_amount; }
    uint32_t value() const { return v_.value; }

public:
    static percent_slim from_string(const string& from);
    string              to_string() const;

    explicit operator string() const {
        return to_string();
    }

    friend inline bool
    operator==(const percent_slim& lhs, const percent_slim& rhs) {
        return lhs.v_ == rhs.v_;
    }

    friend inline bool
    operator!=(const percent_slim& lhs, const percent_slim& rhs) {
        return !(lhs == rhs);
    }

    friend std::ostream&
    operator<<(std::ostream& out, const percent_slim& a) { return out << a.to_string(); }

public:
    friend struct fc::reflector<percent_slim>;

    void
    reflector_init() const {
        EVT_ASSERT(is_amount_within_range(), asset_type_exception, "magnitude of percent_slim amount must be less than 10^5");
    }    

private:
    fc::unsigned_int v_;
};

}}  // namespace evt::chain

namespace fc {

inline void
to_variant(const evt::chain::percent_slim& var, fc::variant& vo) {
    vo = var.to_string();
}

inline void
from_variant(const fc::variant& var, evt::chain::percent_slim& vo) {
    vo = evt::chain::percent_slim::from_string(var.get_string());
}

}  // namespace fc

FC_REFLECT(evt::chain::percent_slim, (v_));
