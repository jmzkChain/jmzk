/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace safemath {

#if defined __GNUC__
#if __GNUC__ >= 7
#define _GCC_BUILDIN_OVERFLOW_P
#endif
#if __GNUC__ >= 5
#define _GCC_BUILDIN_OVERFLOW
#endif
#endif

#ifdef _GCC_BUILDIN_OVERFLOW

template<typename T1, typename T2, typename TR>
inline bool
add(T1 a, T2 b, TR& r) {
    return !__builtin_add_overflow(a, b, &r);
}

template<typename T1, typename T2, typename TR>
inline bool
sub(T1 a, T2 b, TR& r) {
    return !__builtin_sub_overflow(a, b, &r);
}

template<typename T1, typename T2, typename TR>
inline bool
mul(T1 a, T2 b, TR& r) {
    return !__builtin_mul_overflow(a, b, &r);
}

#else
// for now doesn't implement
#endif

#ifdef _GCC_BUILDIN_OVERFLOW_P

template<typename T1, typename T2, typename TR>
inline bool
test_add(T1 a, T2 b, TR& r) {
    return !__builtin_add_overflow_p(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_sub(T1 a, T2 b, TR& r) {
    return !__builtin_sub_overflow_p(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_mul(T1 a, T2 b, TR& r) {
    return !__builtin_mul_overflow_p(a, b, r);
}

#else

#include <SafeInt.hpp>

template<typename T1, typename T2, typename TR>
inline bool
test_add(T1 a, T2 b, TR& r) {
    return SafeAdd(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_sub(T1 a, T2 b, TR& r) {
    return SafeSubtract(a, b, r);
}

template<typename T1, typename T2, typename TR>
inline bool
test_mul(T1 a, T2 b, TR& r) {
    return SafeMultiply(a, b, r);
}

#endif

}} // namespace evt::safemath