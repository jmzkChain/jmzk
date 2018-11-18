/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <stdint.h>
#include <string>
#include <fmt/format.h>
#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/variant.hpp>

namespace evt { namespace chain {

class version {
public:
    version() = default;
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
    int get_version() const { return v_; }

public:
    std::string
    to_string() {
        auto v     = v_;
        int  patch = v % 100;
        v /= 100;
        int minor = v % 100;
        v /= 100;
        int major = v;

        return fmt::format("{}.{}.{}", major, minor, patch);
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

inline void
from_variant(const fc::variant& v, evt::chain::version& version) {
    version.vstr_ = v.get_string();
    std::stringstream ss(version.vstr_);
    std::string item;
    int ver = 0;
    while(std::getline(ss, item, '.')) {
        ver *= 100;
        auto iv = std::stoi(item);
        FC_ASSERT(iv < 100 && iv >= 0, "Not a valid version");
        ver += iv;
    }
    FC_ASSERT(ver >= 0 && ver < 999999, "Not a valid version");
    version.v_ = ver;
}

}  // namespace fc

FC_REFLECT(evt::chain::version, (v_))