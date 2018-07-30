/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <boost/rational.hpp>
#include <evt/chain/asset.hpp>
#include <fc/reflect/variant.hpp>

namespace evt { namespace chain {

string
symbol::to_string() const {
    auto str = fc::to_string(precision());
    str.append(",S#");
    str.append(fc::to_string(id()));
    return str;
}

string
asset::to_string() const {
    auto sign = amount_ < 0 ? "-" : "";
    auto abs_amount = std::abs(amount_);

    auto str = fc::to_string(abs_amount);
    if(precision() >= str.size()) {
        str.insert(0, "0.");
        str.insert(2, precision() - str.size(), '0');
    }
    else {
        str.insert(str.size() - precision(), 1, '.');
    }

    str.append(" S#");
    str.append(fc::to_string(sym_.id()));
    return str;
}

asset
asset::from_string(const string& from) {
    try {
        string s = fc::trim(from);

        // Find space in order to split amount and symbol
        auto space_pos = s.find(' ');
        EVT_ASSERT((space_pos != string::npos), asset_type_exception,
                   "Asset's amount and symbol should be separated with space");
        auto symbol_str = s.substr(space_pos + 1);
        auto amount_str = s.substr(0, space_pos);

        // Ensure that if decimal point is used (.), decimal fraction is specified
        auto dot_pos = amount_str.find('.');
        if(dot_pos != string::npos) {
            EVT_ASSERT((dot_pos != amount_str.size() - 1), asset_type_exception,
                       "Missing decimal fraction after decimal point");
        }

        // Parse symbol
        auto precision = 0u;
        auto sym_id    = fc::to_uint64(symbol_str);

        if(dot_pos != string::npos) {
            precision = amount_str.size() - dot_pos - 1;
        }

        // Parse amount
        safe<int64_t> amount = 0;
        if(dot_pos != string::npos) {
            amount_str.erase(dot_pos, 1);
            amount = fc::to_int64(amount_str);
        }

        return asset(amount.value, symbol(precision, sym_id));
    }
    FC_CAPTURE_LOG_AND_RETHROW((from))
}

}}  // namespace evt::chain
