/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace safemath {

template<typename T1, typename T2, typename TR>
inline bool
add(T1 a, T2 b, TR& r) {
    return __builtin_add_overflow(a, b, &r);
}

template<typename T1, typename T2, typename TR>
inline bool
sub(T1 a, T2 b, TR& r) {
    return __builtin_sub_overflow(a, b, &r);
}

template<typename T1, typename T2, typename TR>
inline bool
mul(T1 a, T2 b, TR& r) {
    return __builtin_mul_overflow(a, b, &r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_add(T1 a, T2 b, TR& r) {
    return __builtin_add_overflow_p(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_sub(T1 a, T2 b, TR& r) {
    return __builtin_sub_overflow_p(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_mul(T1 a, T2 b, TR& r) {
    return __builtin_mul_overflow_p(a, b, r);
}

}}  // namespace evt::safemath