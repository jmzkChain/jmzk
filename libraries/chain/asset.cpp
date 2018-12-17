/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <boost/safe_numerics/safe_integer.hpp>
#include <boost/rational.hpp>
#include <fc/reflect/variant.hpp>
#include <evt/chain/asset.hpp>

namespace evt { namespace chain {

symbol
symbol::from_string(const string& from) {
    try {
        auto s = fc::trim(from);

        // Find comma in order to split precision and symbol id
        auto c = s.find(',');
        EVT_ASSERT(c != string::npos, symbol_type_exception, "Symbol's precision and id should be separated with comma");
        FC_ASSERT(s.substr(c + 1, 2) == "S#");

        auto p  = std::stoul(s.substr(0, c));
        auto id = std::stoul(s.substr(c + 3));

        return symbol(p, id);
    }
    EVT_CAPTURE_AND_RETHROW(symbol_type_exception, (from));
}

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
        auto zeros = precision() - str.size();
        str.insert(0, "0.");
        str.insert(2, zeros, '0');
    }
    else if(precision() > 0) {
        str.insert(str.size() - precision(), 1, '.');
    }

    str.insert(0, sign);

    str.append(" S#");
    str.append(fc::to_string(sym_.id()));
    return str;
}

asset
asset::from_string(const string& from) {
    using namespace boost::safe_numerics;

    try {
        string s = fc::trim(from);

        // Find space in order to split amount and symbol
        auto space_pos = s.find(' ');
        EVT_ASSERT((space_pos != string::npos), asset_type_exception,
                   "Asset's amount and symbol should be separated with space");
        FC_ASSERT(s.substr(space_pos + 1, 2) == "S#");

        auto symbol_str = s.substr(space_pos + 3);
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
        auto amount = safe<int64_t>();
        if(dot_pos != string::npos) {
            amount_str.erase(dot_pos, 1);
        }
        amount = fc::to_int64(amount_str);

        return asset((int64_t)amount, symbol(precision, sym_id));
    }
    EVT_CAPTURE_AND_RETHROW(asset_type_exception, (from));
}

}}  // namespace evt::chain
