#include <sparsehash/dense_hash_map>
#include <fc/io/raw.hpp>

namespace fc { namespace raw {

template<typename Stream, typename K, typename... V>
inline void
pack(Stream& s, const google::dense_hash_map<K, V...>& map) {
    map.serializer([](auto ps, auto& v) {
        fc::raw::pack(*ps, v);
    }, &s);
}

template<typename Stream, typename K, typename V, typename... A>
inline void
unpack(Stream& s, google::dense_hash_map<K, V, A...>& map) {
    return;
}

}}  // namespace fc