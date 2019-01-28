#include <type_traits>
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>
#include <fc/io/raw.hpp>

namespace fc { namespace raw {

template<typename T>
class StreamWrapper {
public:
    StreamWrapper(T& s) : s_(s) {}

public:
    size_t
    Write(const void* data, size_t sz) {
        s_.write((const char*)data, sz);
        return sz;
    }

    size_t
    Read(void* data, size_t sz) {
        s_.read((char*)data, sz);
        return sz;
    }

public:
    T& underlying_stream() { return s_; }

private:
    T& s_;
};

template<typename Stream, typename K, typename V, typename ... Others>
inline void
pack(Stream& s, const google::dense_hash_map<K, V, Others...>& map) {
    FC_ASSERT(map.size() <= MAX_NUM_ARRAY_ELEMENTS);
    // hack here, remove const qualifier
    auto& m = const_cast<google::dense_hash_map<K, V, Others...>&>(map);
    auto  b = StreamWrapper<Stream>(s);
    m.serialize([](auto ps, auto& v) {
        fc::raw::pack(ps->underlying_stream(), v);
        return true;
    }, &b);
}

template<typename Stream, typename K, typename V, typename ... Others>
inline void
unpack(Stream& s, google::dense_hash_map<K, V, Others...>& map) {
    return;
}

}}  // namespace fc