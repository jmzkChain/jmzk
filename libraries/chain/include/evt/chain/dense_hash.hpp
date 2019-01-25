#include <type_traits>
#include <sparsehash/dense_hash_map>
#include <fc/io/raw.hpp>

namespace fc { namespace raw {

template<typename Stream, typename K, typename V, typename ... Others>
inline void
pack(Stream& s, const google::dense_hash_map<K, V, Others...>& map) {
    // hack here, remove const qualifier
    auto& m = const_cast<std::remove_cv_t<decltype(map)>>(map);
    m.serialize([](auto ps, auto& v) {
        fc::raw::pack(*ps, v);
    }, &s);
}

template<typename Stream, typename K, typename V, typename ... Others>
inline void
unpack(Stream& s, google::dense_hash_map<K, V, Others...>& map) {
    return;
}

}}  // namespace fc