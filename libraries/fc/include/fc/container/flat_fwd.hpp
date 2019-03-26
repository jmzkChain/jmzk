#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/vector.hpp>

namespace fc {

using boost::container::flat_map;
using boost::container::flat_set;

namespace raw {

template<typename Stream, typename T, typename... U>
void pack( Stream& s, const boost::container::vector<T, U...>& value );
template<typename Stream, typename T, typename... U>
void unpack( Stream& s, boost::container::vector<T, U...>& value );

template<typename Stream, typename T, typename... U>
void pack(Stream& s, const flat_set<T, U...>& value);
template<typename Stream, typename T, typename... U>
void unpack(Stream& s, flat_set<T, U...>& value);

template<typename Stream, typename K, typename V, typename... U>
void pack(Stream& s, const flat_map<K, V, U...>& value);
template<typename Stream, typename K, typename V, typename... U>
void unpack(Stream& s, flat_map<K, V, U...>& value);

}  // namespace raw
}  // namespace fc
