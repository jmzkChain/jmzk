/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/percent_slim.hpp>

// This fixes the issue in safe_numerics in boost 1.69
#include <evt/chain/workaround/boost/safe_numerics/exception.hpp>
#include <boost/safe_numerics/safe_integer.hpp>

namespace evt { namespace chain {

string
percent_slim::to_string() const {
    if(v_.value == 0) {
        return "0";
    }

    auto str = fc::to_string(v_.value);
    if(kPrecision >= str.size()) {
        auto zeros = kPrecision - str.size();
        str.insert(0, "0.");
        str.insert(2, zeros, '0');
        str.erase(str.find_last_not_of('0') + 1);
    }
    else {
        assert(v_.value == kMaxAmount);
        return "1";
    }

    return str;
}

percent_slim
percent_slim::from_string(const string& from) {
    using namespace boost::safe_numerics;
    static uint32_t _pows[] = { 100'000, 10'000, 1'000, 100, 10, 1 };

    try {
        string s = fc::trim(from);

        // Ensure that if decimal point is used (.), decimal fraction is specified
        auto dot_pos = s.find('.');
        if(dot_pos != string::npos) {
            EVT_ASSERT((dot_pos != s.size() - 1), percent_type_exception,
                       "Missing decimal fraction after decimal point");
        }

        // Parse precision
        auto p = 0u;
        if(dot_pos != string::npos) {
            p = s.size() - dot_pos - 1;
            EVT_ASSERT2(p <= kPrecision, percent_type_exception,
                "Exceed percent's precision: {}", kPrecision);
        }

        // Parse amount
        auto amount = safe<uint32_t>(0);
        if(dot_pos != string::npos) {
            s.erase(dot_pos, 1);
        }
        amount  = fc::to_int64(s);
        amount *= _pows[p];

        return percent_slim((uint32_t)amount);
    }
    EVT_CAPTURE_AND_RETHROW(percent_type_exception, (from));
}

}}  // namespace evt::chain

