#pragma once
#include <boost/container/small_vector.hpp>

namespace fc {

using boost::container::small_vector;
using boost::container::small_vector_base;

namespace raw {

template<typename Stream, typename T, std::size_t N>
inline void pack(Stream& s, const small_vector<T, N>& v);
template<typename Stream, typename T, std::size_t N>
inline void unpack(Stream& s, small_vector<T, N>& v);

}  // namespace raw
}  // namespace fc
