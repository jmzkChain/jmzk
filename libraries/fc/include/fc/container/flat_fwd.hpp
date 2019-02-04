#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/interprocess/containers/vector.hpp>

namespace fc {

using boost::container::flat_map;
using boost::container::flat_set;
namespace bip = boost::interprocess;

namespace raw {

template<typename Stream, typename T, typename Compare, typename Container>
void pack(Stream& s, const flat_set<T, Compare, Container>& value);
template<typename Stream, typename T, typename Compare, typename Container>
void unpack(Stream& s, flat_set<T, Compare, Container>& value);
template<typename Stream, typename K, typename V, typename ... Others>
void pack(Stream& s, const flat_map<K, V, Others...>& value);
template<typename Stream, typename K, typename V, typename ... Others>
void unpack(Stream& s, flat_map<K, V, Others...>& value);

template<typename Stream, typename T, typename A>
void pack(Stream& s, const bip::vector<T, A>& value);
template<typename Stream, typename T, typename A>
void unpack(Stream& s, bip::vector<T, A>& value);

}  // namespace raw
}  // namespace fc
