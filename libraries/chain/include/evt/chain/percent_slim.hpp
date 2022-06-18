/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <fmt/format.h>
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

class percent_slim : public fc::reflect_init {
public:
    static constexpr uint32_t kMaxAmount = 100'000u;
    static constexpr uint32_t kPrecision = 5;

public:
    percent_slim() = default;
    percent_slim(const percent_slim&) = default;

    explicit percent_slim(uint32_t v)
        : v_(v) {
        jmzk_ASSERT(is_amount_within_range(), percent_type_exception, "magnitude of percent_slim value must be less than 10^5");
    }

    percent_slim(const percent_type& p) {
        v_ = (uint32_t)(p * 100'000u);
    }

public:
    bool is_amount_within_range() const { return v_.value <= kMaxAmount; }
    percent_type value() const { return percent_type(v_.value) / 100'000u; }
    uint32_t raw_value() const { return v_.value; }
    explicit operator percent_type() const { return value(); }

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
        jmzk_ASSERT(is_amount_within_range(), percent_type_exception, "magnitude of percent_slim amount must be less than 10^5");
    }    

private:
    fc::unsigned_int v_;
};

}}  // namespace jmzk::chain

namespace fc {

inline void
to_variant(const jmzk::chain::percent_slim& var, fc::variant& vo) {
    vo = var.to_string();
}

inline void
from_variant(const fc::variant& var, jmzk::chain::percent_slim& vo) {
    vo = jmzk::chain::percent_slim::from_string(var.get_string());
}

}  // namespace fc

FC_REFLECT(jmzk::chain::percent_slim, (v_));
