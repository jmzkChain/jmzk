#pragma once
#include <fc/variant.hpp>
#include <fc/container/small_vector_fwd.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc {
namespace raw {

template<typename Stream, typename T, std::size_t N>
inline void
pack(Stream& s, const small_vector<T,N>& v) {
    FC_ASSERT(v.size() <= MAX_NUM_ARRAY_ELEMENTS);
    fc::raw::pack(s, unsigned_int((uint32_t)v.size()));
    
    for(auto& e : v) {
        fc::raw::pack(s, e);
    }
}

template<typename Stream, typename T, std::size_t N>
inline void
unpack(Stream& s, small_vector<T,N>& v) {
    auto size = unsigned_int();
    fc::raw::unpack(s, size);
    FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);

    v.resize(size.value);
    for(auto& e : v) {
        fc::raw::unpack(s, e);
    }
}

}  // namesapce raw

template<typename T, std::size_t N>
void
to_variant(const small_vector<T,N>& var,  variant& vo)
{
    auto vars = variants();
    vars.resize(var.size());

    int i = 0;
    for(auto& e : var) {
        vars[i++] = variant(e);
    }
    vo = vars;
}

template<typename T, std::size_t N>
void
from_variant(const variant& var,  small_vector<T,N>& vo)
{
    const auto& vars = var.get_array();

    vo.clear();
    vo.reserve(vars.size());

    for(auto& v : vars) {
        vo.emplace_back(v.as<T>());
    }
}

}  // namesapce fc

