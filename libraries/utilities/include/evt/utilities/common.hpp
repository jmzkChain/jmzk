/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 *
 */
#pragma once

#ifndef COMMON_HPP
#define COMMON_HPP

namespace evt { namespace utilities { namespace common {
  template<typename I>
  std::string itoh(I n, size_t hlen = sizeof(I)<<1) {
     static const char* digits = "0123456789abcdef";
     std::string r(hlen, '0');
     for(size_t i = 0, j = (hlen - 1) * 4 ; i < hlen; ++i, j -= 4)
        r[i] = digits[(n>>j) & 0x0f];
     return r;
  }

template<typename Storage>
struct eq_comparator {
    struct visitor : public fc::visitor<bool> {
        visitor(const Storage &b)
            : b_(b) {}

        template<typename ValueType>
        bool
        operator()(const ValueType& a) const {
            const auto& b = b_.template get<ValueType>();
            return a == b;
        }

        const Storage& b_;
    };

    static bool
    apply(const Storage& a, const Storage& b) {
        return a.which() == b.which() && a.visit(visitor(b));
    }
};

}}}

#endif // COMMON_HPP
