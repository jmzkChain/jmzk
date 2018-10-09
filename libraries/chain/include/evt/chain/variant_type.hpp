/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <boost/hana/for_each.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/integral_constant.hpp>
#include <fc/static_variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {

template<typename ENUM, int MAX, typename... ARGS>
struct variant_type {
public:
    template<typename T>
    T& get() {
        return value_.get<T>();
    }

    template<typename T>
    const T& get() const {
        return value_.get<T>();
    }

    ENUM
    type() const {
        return (ENUM)value_.which();
    }

private:
    fc::static_variant<ARGS...> value_;
};

}}  // namespace evt::chain

namespace fc {

template<typename ENUM, int MAX, typename... ARGS>
void
to_variant(const variant_type<ENUM, MAX, ARGS...>& vo, variant& var) {
    using namespace boost;

    auto type  = var["type"].as_int64(); 
    EVT_ASSERT(type <= MAX, variant_type_exception, "Type index is not valid");

    auto range = hana::range_c<int, 0, MAX + 1>;
    hana::for_each(range, [&](auto i) {
        if(type == i) {
            using objt = decltype(vo.value_)::type_at<i>;
            
        }
    });
}

template<typename ENUM, int MAX, typename... ARGS>
void
from_variant(const variant& var, variant_type<ENUM, MAX, ARGS...>& vo) {
    
}

}  // namespace fc

FC_REFLECT(evt::chain::variant_type, (value_));