/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>
#include <stdint.h>
#include <string>

namespace evt { namespace chain {

class version {
public:
    version(int major, int minor, int patch)
        : v_(major * 10000 + minor * 100 + patch) {
        FC_ASSERT(major >= 0 && major <= 99, "Not valid version");
        FC_ASSERT(minor >= 0 && minor <= 99, "Not valid version");
        FC_ASSERT(patch >= 0 && patch <= 99, "Not valid version");
        vstr_ = to_string();
    }

public:
    int get_major()  const { return v_ / 10000; }
    int get_minor()  const { return (v_ / 100) % 100; }
    int get_patch()  const { return v_ % 100; }
    int get_verion() const { return v_; }

public:
    std::string
    to_string() {
        auto v     = v_;
        int  patch = v % 100;
        v /= 100;
        int minor = v % 100;
        v /= 100;
        int major = v;

        std::string vstr;
        vstr.reserve(10);
        vstr.append(std::to_string(major));
        vstr.push_back('.');
        vstr.append(std::to_string(minor));
        vstr.push_back('.');
        vstr.append(std::to_string(patch));
        return vstr;
    }

public:
    int         v_;
    std::string vstr_;
};

}}  // namespace evt::chain

namespace fc {

inline void
to_variant(const evt::chain::version& version, fc::variant& v) {
    v = version.vstr_;
}

}  // namespace fc

FC_REFLECT(evt::chain::version, (v_))