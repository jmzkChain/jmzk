#pragma once
#include <array>
#include <deque>
#include <map>
#include <optional>
#include <tuple>

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

#include <boost/hana/for_each.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/integral_constant.hpp>

#include <fc/io/raw_fwd.hpp>
#include <fc/io/raw_variant.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/varint.hpp>
#include <fc/variant_wrapper.hpp>
#include <fc/fwd.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/exception/exception.hpp>

namespace fc {
namespace raw {

namespace bip = boost::interprocess;

using shared_string = bip::basic_string<char, std::char_traits<char>, bip::allocator<char, bip::managed_mapped_file::segment_manager>>;

template<typename T>
struct packer {
    enum { empty = 0 };
};

template<typename T>
struct unpacker {
    enum { empty = 0 };
};

template<typename T, typename R = decltype(packer<T>::empty), typename S = decltype(unpacker<T>::empty)>
constexpr bool
has_custom_function_helper(int) {
    return false;
}

template<typename T>
constexpr bool
has_custom_function_helper(...) {
    return true;
}

template<typename T>
constexpr bool
has_custom_function() {
    return has_custom_function_helper<T>(0);
}

template<typename Stream, typename Arg0, typename... Args>
inline void
pack(Stream& s, const Arg0& a0, Args... args) {
    pack(s, a0);
    pack(s, args...);
}

template<typename Stream>
inline void
pack(Stream& s, const fc::exception& e) {
    fc::raw::pack(s, e.code());
    fc::raw::pack(s, std::string(e.name()));
    fc::raw::pack(s, std::string(e.what()));
    fc::raw::pack(s, e.get_log());
}

template<typename Stream>
inline void
unpack(Stream& s, fc::exception& e) {
    int64_t      code;
    std::string  name, what;
    log_messages msgs;

    fc::raw::unpack(s, code);
    fc::raw::unpack(s, name);
    fc::raw::unpack(s, what);
    fc::raw::unpack(s, msgs);

    e = fc::exception(fc::move(msgs), code, name, what);
}

template<typename Stream>
inline void
pack(Stream& s, const fc::log_message& msg) {
    fc::raw::pack(s, variant(msg));
}

template<typename Stream>
inline void
unpack(Stream& s, fc::log_message& msg) {
    fc::variant vmsg;
    fc::raw::unpack(s, vmsg);
    msg = vmsg.as<log_message>();
}

template<typename Stream>
inline void
pack(Stream& s, const fc::path& tp) {
    fc::raw::pack(s, tp.generic_string());
}

template<typename Stream>
inline void
unpack(Stream& s, fc::path& tp) {
    std::string p;
    fc::raw::unpack(s, p);
    tp = p;
}

template<typename Stream>
inline void
pack(Stream& s, const fc::time_point_sec& tp) {
    uint32_t usec = tp.sec_since_epoch();
    s.write((const char*)&usec, sizeof(usec));
}

template<typename Stream>
inline void
unpack(Stream& s, fc::time_point_sec& tp) {
    try {
        uint32_t sec;
        s.read((char*)&sec, sizeof(sec));
        tp = fc::time_point() + fc::seconds(sec);
    }
    FC_RETHROW_EXCEPTIONS(warn, "")
}

template<typename Stream>
inline void
pack(Stream& s, const fc::time_point& tp) {
    uint64_t usec = tp.time_since_epoch().count();
    s.write((const char*)&usec, sizeof(usec));
}

template<typename Stream>
inline void
unpack(Stream& s, fc::time_point& tp) {
    try {
        uint64_t usec;
        s.read((char*)&usec, sizeof(usec));
        tp = fc::time_point() + fc::microseconds(usec);
    }
    FC_RETHROW_EXCEPTIONS(warn, "")
}

template<typename Stream>
inline void
pack(Stream& s, const fc::microseconds& usec) {
    uint64_t usec_as_int64 = usec.count();
    s.write((const char*)&usec_as_int64, sizeof(usec_as_int64));
}

template<typename Stream>
inline void
unpack(Stream& s, fc::microseconds& usec) {
    try {
        uint64_t usec_as_int64;
        s.read((char*)&usec_as_int64, sizeof(usec_as_int64));
        usec = fc::microseconds(usec_as_int64);
    }
    FC_RETHROW_EXCEPTIONS(warn, "")
}

template<typename Stream, typename T, size_t N>
inline void
pack(Stream& s, T (&v)[N]) {
    static_assert(N <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");

    fc::raw::pack(s, unsigned_int((uint32_t)N));
    for(uint64_t i = 0; i < N; ++i) {
        fc::raw::pack(s, v[i]);
    }
}

template<typename Stream, typename T, size_t N>
inline void
unpack(Stream& s, T (&v)[N]) {
    static_assert(N <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");

    try {
        unsigned_int size;
        fc::raw::unpack(s, size);
        FC_ASSERT(size.value == N);
        for(uint64_t i = 0; i < N; ++i) {
            fc::raw::unpack(s, v[i]);
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "${type} (&v)[${length}]", ("type", fc::get_typename<T>::name())("length", N))
}

template<typename Stream, typename T>
inline void
pack(Stream& s, const std::shared_ptr<T>& v) {
    fc::raw::pack(s, bool(!!v));
    if(!!v)
        fc::raw::pack(s, *v);
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, std::shared_ptr<T>& v) {
    try {
        bool b;
        fc::raw::unpack(s, b);
        if(b) {
            v = std::make_shared<T>();
            fc::raw::unpack(s, *v);
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "std::shared_ptr<T>", ("type", fc::get_typename<T>::name()))
}

template<typename Stream>
inline void
pack(Stream& s, const signed_int& v) {
    uint32_t val = (v.value<<1) ^ (v.value>>31); //apply zigzag encoding
    do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b, 1);  //.put(b);
    } while(val);
}

template<typename Stream>
inline void
pack(Stream& s, const unsigned_int& v) {
    uint64_t val = v.value;
    do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b, 1);  //.put(b);
    } while(val);
}

template<typename Stream>
inline void
unpack(Stream& s, signed_int& vi) {
    uint32_t v  = 0;
    char     b  = 0;
    int      by = 0;
    do {
        s.get(b);
        v |= uint32_t(uint8_t(b) & 0x7f) << by;
        by += 7;
    } while(uint8_t(b) & 0x80);
    vi.value = (v>>1) ^ (~(v&1)+1ull); //reverse zigzag encoding
}

template<typename Stream>
inline void
unpack(Stream& s, unsigned_int& vi) {
    uint64_t v  = 0;
    char     b  = 0;
    uint8_t  by = 0;
    do {
        s.get(b);
        v |= uint32_t(uint8_t(b) & 0x7f) << by;
        by += 7;
    } while(uint8_t(b) & 0x80 && by < 32);
    vi.value = static_cast<uint32_t>(v);
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, const T& vi) {
    T tmp;
    fc::raw::unpack(s, tmp);
    FC_ASSERT(vi == tmp);
}

template<typename Stream>
inline void
pack(Stream& s, const char* v) {
    fc::raw::pack(s, fc::string(v));
}

template<typename Stream, typename T, unsigned int S, typename Align>
void
pack(Stream& s, const fc::fwd<T, S, Align>& v) {
    fc::raw::pack(*v);
}

template<typename Stream, typename T, unsigned int S, typename Align>
void
unpack(Stream& s, fc::fwd<T, S, Align>& v) {
    fc::raw::unpack(*v);
}

template<typename Stream, typename T>
void
pack(Stream& s, const fc::smart_ref<T>& v) {
    fc::raw::pack(s, *v);
}

template<typename Stream, typename T>
void
unpack(Stream& s, fc::smart_ref<T>& v) {
    fc::raw::unpack(s, *v);
}

// optional
template<typename Stream, typename T>
void
pack(Stream& s, const std::optional<T>& v) {
    fc::raw::pack(s, bool(v.has_value()));
    if(v.has_value())
        fc::raw::pack(s, *v);
}

template<typename Stream, typename T>
void
unpack(Stream& s, std::optional<T>& v) {
    try {
        bool b;
        fc::raw::unpack(s, b);
        if(b) {
            v = T();
            fc::raw::unpack(s, *v);
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "optional<${type}>", ("type", fc::get_typename<T>::name()))
}

// std::vector<char>
template<typename Stream>
inline void
pack(Stream& s, const std::vector<char>& value) {
    FC_ASSERT(value.size() <= MAX_SIZE_OF_BYTE_ARRAYS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    if(value.size()) {
        s.write(&value.front(), (uint32_t)value.size());
    }
}

template<typename Stream>
inline void
unpack(Stream& s, std::vector<char>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_SIZE_OF_BYTE_ARRAYS);

    value.resize(size.value);
    if(value.size()) {
        s.read(value.data(), value.size());
    }
}

// std::string
template<typename Stream>
inline void
pack(Stream& s, const std::string& v) {
    FC_ASSERT(v.size() <= MAX_SIZE_OF_BYTE_ARRAYS);
    fc::raw::pack(s, unsigned_int((uint32_t)v.size()));
    if(v.size()) {
        s.write(v.data(), v.size());
    }
}

template<typename Stream>
inline void
unpack(Stream& s, std::string& v) {
    auto sz = unsigned_int();
    fc::raw::unpack(s, sz);
    FC_ASSERT(sz.value <= MAX_SIZE_OF_BYTE_ARRAYS);

    v.resize((uint32_t)sz);
    if(v.size()) {
        s.read(v.data(), v.size());
    }
}

// bip::basic_string
template<typename Stream>
inline void
pack(Stream& s, const shared_string& v) {
    FC_ASSERT(v.size() <= MAX_SIZE_OF_BYTE_ARRAYS);
    fc::raw::pack(s, unsigned_int((uint32_t)v.size()));
    if(v.size()) {
        s.write(v.c_str(), v.size());
    }
}

template<typename Stream>
inline void
unpack(Stream& s, shared_string& v) {
    std::vector<char> tmp;
    fc::raw::unpack(s, tmp);
    FC_ASSERT(v.size() == 0);

    if(tmp.size()) {
        v.append(tmp.begin(), tmp.end());
    }
}

// bool
template<typename Stream>
inline void
pack(Stream& s, const bool& v) {
    fc::raw::pack(s, uint8_t(v));
}

template<typename Stream>
inline void
unpack(Stream& s, bool& v) {
    uint8_t b;
    fc::raw::unpack(s, b);
    FC_ASSERT((b & ~1) == 0);
    v = (b != 0);
}

namespace detail {

template<typename Stream, typename Class>
struct pack_object_visitor {
    pack_object_visitor(const Class& _c, Stream& _s)
        : c(_c)
        , s(_s) {}

    template<typename T, typename C, T(C::*p)>
    void operator()(const char* name) const {
        fc::raw::pack(s, c.*p);
    }

private:
    const Class& c;
    Stream&      s;
};

template<typename Stream, typename Class>
struct unpack_object_visitor : public fc::reflector_init_visitor<Class> {
    unpack_object_visitor(Class& _c, Stream& _s)
        : fc::reflector_init_visitor<Class>(_c)
        , s(_s) {}

    template<typename T, typename C, T(C::*p)>
    inline void operator()(const char* name) const {
        try {
            fc::raw::unpack(s, this->obj.*p);
        }
        FC_RETHROW_EXCEPTIONS(warn, "Error unpacking field ${field}", ("field", name))
    }

private:
    Stream& s;
};

template<typename HasCustom = fc::true_type>
struct has_custom {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) { packer<T>::pack(s, v); }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) { unpacker<T>::unpack(s, v); }
};

template<>
struct has_custom<fc::false_type> {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) { s << v; }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) { s >> v; }
};

template<typename IsClass = fc::true_type>
struct if_class {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        has_custom<std::conditional_t<has_custom_function<T>(), fc::true_type, fc::false_type>>::pack(s, v);
    }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        has_custom<std::conditional_t<has_custom_function<T>(), fc::true_type, fc::false_type>>::unpack(s, v);
    }
};

template<>
struct if_class<fc::false_type> {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        s.write((char*)&v, sizeof(v));
    }
    
    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        s.read((char*)&v, sizeof(v));
    }
};

template<typename IsEnum = fc::false_type>
struct if_enum {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        fc::reflector<T>::visit(pack_object_visitor<Stream, T>(v, s));
    }
    
    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        fc::reflector<T>::visit(unpack_object_visitor<Stream, T>(v, s));
    }
};

template<>
struct if_enum<fc::true_type> {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        fc::raw::pack(s, (int64_t)v);
    }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        int64_t temp;
        fc::raw::unpack(s, temp);
        v = (T)temp;
    }
};

template<typename IsReflected = fc::false_type>
struct if_reflected {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        if_class<typename fc::is_class<T>::type>::pack(s, v);
    }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        if_class<typename fc::is_class<T>::type>::unpack(s, v);
    }
};

template<>
struct if_reflected<fc::true_type> {
    template<typename Stream, typename T>
    static inline void pack(Stream& s, const T& v) {
        if_enum<typename fc::reflector<T>::is_enum>::pack(s, v);
    }

    template<typename Stream, typename T>
    static inline void unpack(Stream& s, T& v) {
        if_enum<typename fc::reflector<T>::is_enum>::unpack(s, v);
    }
};

}  // namespace detail

// allow users to verify version of fc calls reflector_init on unpacked reflected types
constexpr bool has_feature_reflector_init_on_unpacked_reflected_types = true;

template<typename Stream, typename T>
inline void
pack(Stream& s, const std::unordered_set<T>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, std::unordered_set<T>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    value.clear();
    value.reserve(size.value);
    for(uint32_t i = 0; i < size.value; ++i) {
        T tmp;
        fc::raw::unpack(s, tmp);
        value.insert(std::move(tmp));
    }
}

template<typename Stream, typename K, typename V>
inline void
pack(Stream& s, const std::pair<K, V>& value) {
    fc::raw::pack(s, value.first);
    fc::raw::pack(s, value.second);
}

template<typename Stream, typename K, typename V>
inline void
unpack(Stream& s, std::pair<K, V>& value) {
    fc::raw::unpack(s, value.first);
    fc::raw::unpack(s, value.second);
}

namespace detail {

template<typename Stream, int N = 0, typename... Types>
inline void
pack(Stream& s, const std::tuple<Types...>& value) {
    fc::raw::pack(s, std::get<N>(value));
    if constexpr(N + 1 < std::tuple_size<std::remove_reference_t<decltype(value)>>::value) {
        pack<Stream, N + 1, Types...>(s, value);
    }
}

template<typename Stream, int N = 0, typename... Types>
inline void
unpack(Stream& s, std::tuple<Types...>& value) {
    fc::raw::unpack(s, std::get<N>(value));
    if constexpr(N + 1 < std::tuple_size<std::remove_reference_t<decltype(value)>>::value) {
        unpack<Stream, N + 1, Types...>(s, value);
    }
}

}  // namespace detail

template<typename Stream, typename... Types>
inline void
pack(Stream& s, const std::tuple<Types...>& value) {
    detail::pack<Stream, 0, Types...>(s, value);
}

template<typename Stream, typename... Types>
inline void
unpack(Stream& s, std::tuple<Types...>& value) {
    detail::unpack<Stream, 0, Types...>(s, value);
}

template<typename Stream, typename K, typename V, typename ... Others>
inline void
pack(Stream& s, const std::unordered_map<K, V, Others...>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename K, typename V, typename ... Others>
inline void
unpack(Stream& s, std::unordered_map<K, V, Others...>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    value.clear();
    value.reserve(size.value);
    for(uint32_t i = 0; i < size.value; ++i) {
        std::pair<K, V> tmp;
        fc::raw::unpack(s, tmp);
        value.insert(std::move(tmp));
    }
}

template<typename Stream, typename K, typename V>
inline void
pack(Stream& s, const std::map<K, V>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename K, typename V>
inline void
unpack(Stream& s, std::map<K, V>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    value.clear();
    for(uint32_t i = 0; i < size.value; ++i) {
        std::pair<K, V> tmp;
        fc::raw::unpack(s, tmp);
        value.insert(std::move(tmp));
    }
}

template<typename Stream, typename T>
inline void
pack(Stream& s, const std::deque<T>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, std::deque<T>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    value.resize(size.value);
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::unpack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
pack(Stream& s, const std::vector<T>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, std::vector<T>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    value.resize(size.value);
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::unpack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
pack(Stream& s, const std::set<T>& value) {
    FC_ASSERT(value.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)value.size()));
    auto itr = value.begin();
    auto end = value.end();
    while(itr != end) {
        fc::raw::pack(s, *itr);
        ++itr;
    }
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, std::set<T>& value) {
    unsigned_int size;
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
    for(uint64_t i = 0; i < size.value; ++i) {
        T tmp;
        fc::raw::unpack(s, tmp);
        value.insert(std::move(tmp));
    }
}

template<typename Stream, typename T, std::size_t S>
inline auto
pack(Stream& s, const std::array<T, S>& value) -> std::enable_if_t<std::is_trivially_copyable_v<T>> {
    static_assert(S <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");
    s.write((const char*)value.data(), S * sizeof(T));
}

template<typename Stream, typename T, std::size_t S>
inline auto
pack(Stream& s, const std::array<T, S>& value) -> std::enable_if_t<!std::is_trivially_copyable_v<T>> {
    static_assert(S <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");
    for(std::size_t i = 0; i < S; ++i) {
        fc::raw::pack(s, value[i]);
    }
}

template<typename Stream, typename T, std::size_t S>
inline auto
unpack(Stream& s, std::array<T, S>& value) -> std::enable_if_t<std::is_trivially_copyable_v<T>> {
    static_assert(S <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");
    s.read((char*)value.data(), S * sizeof(T));
}

template<typename Stream, typename T, std::size_t S>
inline auto
unpack(Stream& s, std::array<T, S>& value) -> std::enable_if_t<!std::is_trivially_copyable_v<T>> {
    static_assert(S <= MAX_NUM_ARRAY_ELEMENTS, "number of elements in array is too large");
    for(std::size_t i = 0; i < S; ++i) {
        fc::raw::unpack(s, value[i]);
    }
}

template<typename Stream, typename T>
inline void
pack(Stream& s, const T& v) {
    fc::raw::detail::if_reflected<typename fc::reflector<T>::is_defined>::pack(s, v);
}

template<typename Stream, typename T>
inline void
unpack(Stream& s, T& v) {
    try {
        fc::raw::detail::if_reflected<typename fc::reflector<T>::is_defined>::unpack(s, v);
    }
    FC_RETHROW_EXCEPTIONS(warn, "error unpacking ${type}", ("type", fc::get_typename<T>::name()))
}

template<typename T>
inline size_t
pack_size(const T& v) {
    datastream<size_t> ps;
    fc::raw::pack(ps, v);
    return ps.tellp();
}

template<typename T>
inline std::vector<char>
pack(const T& v) {
    datastream<size_t> ps;
    fc::raw::pack(ps, v);
    std::vector<char> vec(ps.tellp());

    if(vec.size()) {
        datastream<char*> ds(vec.data(), size_t(vec.size()));
        fc::raw::pack(ds, v);
    }
    return vec;
}

template<typename T, typename... Next>
inline std::vector<char>
pack(const T& v, Next... next) {
    datastream<size_t> ps;
    fc::raw::pack(ps, v, next...);
    std::vector<char> vec(ps.tellp());

    if(vec.size()) {
        datastream<char*> ds(vec.data(), size_t(vec.size()));
        fc::raw::pack(ds, v, next...);
    }
    return vec;
}

template<typename T>
inline T
unpack(const std::vector<char>& s) {
    try {
        T tmp;
        auto ds = datastream<const char*>(s.data(), s.size());
        fc::raw::unpack(ds, tmp);
        if(ds.remaining() > 0) {
            FC_THROW_EXCEPTION2(raw_unpack_exception, "Binary buffer is not EOF after unpack variable, remaining: {} bytes.", ds.remaining());
        }
        return tmp;
    }
    FC_RETHROW_EXCEPTIONS(warn, "error unpacking ${type}", ("type", fc::get_typename<T>::name()))
}

template<typename T>
inline void
unpack(const std::vector<char>& s, T& tmp) {
    try {
        auto ds = datastream<const char*>(s.data(), s.size());
        fc::raw::unpack(ds, tmp);
        if(ds.remaining() > 0) {
            FC_THROW_EXCEPTION2(raw_unpack_exception, "Binary buffer is not EOF after unpack variable, remaining: {} bytes.", ds.remaining());
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "error unpacking ${type}", ("type", fc::get_typename<T>::name()))
}

template<typename T>
inline T
unpack(const char* d, uint32_t s) {
    try {
        T v;
        auto ds = datastream<const char*>(d, s);
        fc::raw::unpack(ds, v);
        if(ds.remaining() > 0) {
            FC_THROW_EXCEPTION2(raw_unpack_exception, "Binary buffer is not EOF after unpack variable, remaining: {} bytes.", ds.remaining());
        }
        return v;
    }
    FC_RETHROW_EXCEPTIONS(warn, "error unpacking ${type}", ("type", fc::get_typename<T>::name()))
}

template<typename T>
inline void
unpack(const char* d, uint32_t s, T& v) {
    try {
        auto ds = datastream<const char*>(d, s);
        fc::raw::unpack(ds, v);
        if(ds.remaining() > 0) {
            FC_THROW_EXCEPTION2(raw_unpack_exception, "Binary buffer is not EOF after unpack variable, remaining: {} bytes.", ds.remaining());
        }
    }
    FC_RETHROW_EXCEPTIONS(warn, "error unpacking ${type}", ("type", fc::get_typename<T>::name()))
}

template<typename T>
inline void
pack(char* d, uint32_t s, const T& v) {
    datastream<char*> ds(d, s);
    fc::raw::pack(ds, v);
}

template<typename Stream>
struct pack_static_variant {
    Stream& stream;
    pack_static_variant(Stream& s)
        : stream(s) {}

    typedef void result_type;
    template<typename T>
    void operator()(const T& v) const {
        fc::raw::pack(stream, v);
    }
};

template<typename Stream>
struct unpack_static_variant {
    Stream& stream;
    unpack_static_variant(Stream& s)
        : stream(s) {}

    typedef void result_type;
    template<typename T>
    void operator()(T& v) const {
        fc::raw::unpack(stream, v);
    }
};

template<typename Stream, typename... T>
void
pack(Stream& s, const static_variant<T...>& sv) {
    fc::raw::pack(s, unsigned_int(sv.which()));
    sv.visit(pack_static_variant<Stream>(s));
}

template<typename Stream, typename... T>
void
unpack(Stream& s, static_variant<T...>& sv) {
    unsigned_int w;
    fc::raw::unpack(s, w);
    sv.set_which(w.value);
    sv.visit(unpack_static_variant<Stream>(s));
}

template<typename Stream, typename... T>
void
pack(Stream& s, const std::variant<T...>& var) {
    fc::raw::pack(s, unsigned_int(var.index()));
    std::visit([&s](auto& v) {
        fc::raw::pack(s, v);
    }, var);
}

template<typename Stream, typename... T>
void
unpack(Stream& s, std::variant<T...>& var) {
    unsigned_int w;
    fc::raw::unpack(s, w);

    auto range = boost::hana::range_c<int, 0, sizeof...(T)>;
    boost::hana::for_each(range, [&](auto i) {
        if((int)w == i()) {
            using obj_t = std::variant_alternative_t<i(), std::decay_t<decltype(var)>>;
            auto obj    = obj_t{};

            fc::raw::unpack(s, obj);
            var.template emplace<obj_t>(std::move(obj));
        }
    });
}

template<typename Stream, typename ENUM, typename... ARGS>
void
pack(Stream& s, const variant_wrapper<ENUM, ARGS...>& vw) {
    fc::raw::pack(s, vw.value_);
}

template<typename Stream, typename ENUM, typename... ARGS>
void
unpack(Stream& s, variant_wrapper<ENUM, ARGS...>& vw) {
    fc::raw::unpack(s, vw.value_);
}

template<typename Stream, typename T>
void
pack(Stream& s, const boost::multiprecision::number<T>& n) {
    // work around, don't use raw number anymore
    auto b  = n.str();
    auto n2 = boost::multiprecision::number<T>();
    memset((char*)&n2, 0, sizeof(n2));
    n2 = boost::multiprecision::number<T>(b);

    s.write((const char*)&n2, sizeof(n2));
}

template<typename Stream, typename T>
void
unpack(Stream& s, boost::multiprecision::number<T>& n) {
    s.read((char*)&n, sizeof(n));
}

}}  // namespace fc::raw
