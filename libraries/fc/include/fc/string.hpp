#pragma once
#include <optional>
#include <string>
#include <fc/utility.hpp>
#include <fc/fwd.hpp>

namespace fc {
typedef std::string string;

int64_t     to_int64(const fc::string&);
uint64_t    to_uint64(const fc::string&);
double      to_double(const fc::string&);
fc::string  to_string(double);
fc::string  to_string(uint64_t);
fc::string  to_string(int64_t);
fc::string  to_string(uint16_t);
std::string to_pretty_string(int64_t);

inline fc::string
to_string(int32_t v) {
    return to_string(int64_t(v));
}

inline fc::string
to_string(uint32_t v) {
    return to_string(uint64_t(v));
}

#ifdef __APPLE__

inline fc::string
to_string(size_t s) {
    return to_string(uint64_t(s));
}

#endif

typedef std::optional<fc::string> ostring;
class variant_object;
fc::string format_string(const fc::string&, const variant_object&);
fc::string trim(const fc::string&);
fc::string to_lower(const fc::string&);
string     trim_and_normalize_spaces(const string& s);

}  // namespace fc
