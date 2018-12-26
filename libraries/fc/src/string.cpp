#include <fc/string.hpp>
#include <fc/utility.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <locale>
#include <limits>

/**
 *  Implemented with std::string for now.
 */
namespace fc {

class comma_numpunct : public std::numpunct<char> {
protected:
    virtual char        do_thousands_sep() const { return ','; }
    virtual std::string do_grouping() const { return "\03"; }
};

std::string
to_pretty_string(int64_t value) {
    std::stringstream ss;
    ss.imbue({std::locale(), new comma_numpunct});
    ss << std::fixed << value;
    return ss.str();
}

int64_t
to_int64(const fc::string& i) {
    try {
        return boost::lexical_cast<int64_t>(i.c_str(), i.size());
    }
    catch(const boost::bad_lexical_cast& e) {
        FC_THROW_EXCEPTION(parse_error_exception, "Couldn't parse int64_t");
    }
    FC_RETHROW_EXCEPTIONS(warn, "${i} => int64_t", ("i", i))
}

uint64_t
to_uint64(const fc::string& i) {
    try {
        try {
            return boost::lexical_cast<uint64_t>(i.c_str(), i.size());
        }
        catch(const boost::bad_lexical_cast& e) {
            FC_THROW_EXCEPTION(parse_error_exception, "Couldn't parse uint64_t");
        }
        FC_RETHROW_EXCEPTIONS(warn, "${i} => uint64_t", ("i", i))
    }
    FC_CAPTURE_AND_RETHROW((i))
}

double
to_double(const fc::string& i) {
    try {
        return boost::lexical_cast<double>(i.c_str(), i.size());
    }
    catch(const boost::bad_lexical_cast& e) {
        FC_THROW_EXCEPTION(parse_error_exception, "Couldn't parse double");
    }
    FC_RETHROW_EXCEPTIONS(warn, "${i} => double", ("i", i))
}

fc::string
to_string(double d) {
    // +2 is required to ensure that the double is rounded correctly when read back in.  http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
    std::stringstream ss;
    ss << std::setprecision(std::numeric_limits<double>::digits10 + 2) << std::fixed << d;
    return ss.str();
}

fc::string
to_string(uint64_t d) {
    return boost::lexical_cast<std::string>(d);
}

fc::string
to_string(int64_t d) {
    return boost::lexical_cast<std::string>(d);
}

fc::string
to_string(uint16_t d) {
    return boost::lexical_cast<std::string>(d);
}

std::string
trim(const std::string& s) {
    return boost::algorithm::trim_copy(s);
    /*
      std::string cpy(s);
      boost::algorithm::trim(cpy);
      return cpy;
      */
}

std::string
to_lower(const std::string& s) {
    auto tmp = s;
    boost::algorithm::to_lower(tmp);
    return tmp;
}

string
trim_and_normalize_spaces(const string& s) {
    string result = boost::algorithm::trim_copy(s);
    while(result.find("  ") != result.npos)
        boost::algorithm::replace_all(result, "  ", " ");
    return result;
}

}  // namespace fc
