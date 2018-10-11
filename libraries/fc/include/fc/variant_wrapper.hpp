/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <boost/core/typeinfo.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/integral_constant.hpp>
#include <fc/static_variant.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

namespace fc {

template<typename ENUM, typename... ARGS>
struct variant_wrapper {
public:
    variant_wrapper() {
        static_assert((size_t)ENUM::max_value + 1 == sizeof...(ARGS));  // type index starts with 0
    }

    template<typename T, typename = typename std::enable_if<!std::is_same_v<variant_wrapper, std::decay_t<T>>>::type>
    variant_wrapper(T&& v) {
        value_ = std::forward<T>(v);
    }

public:
    template<typename X>
    X&
    get() {
        return value_.template get<X>();
    }

    template<typename X>
    const X&
    get() const {
        return value_.template get<X>();
    }

    ENUM
    type() const {
        return (ENUM)value_.which();
    }

public:
    fc::static_variant<ARGS...> value_;
};

template<typename ENUM, typename... ARGS>
void
to_variant(const variant_wrapper<ENUM, ARGS...>& vo, variant& var) {
    using namespace boost;

    FC_ASSERT((int)vo.type() <= (int)ENUM::max_value, "Type index is not valid");
    auto mo = mutable_variant_object();
    mo["type"] = (int)vo.type();

    auto range = hana::range_c<int, 0, (int)ENUM::max_value + 1>;
    hana::for_each(range, [&](auto i) {
        if((int)vo.type() == i()) {
            using obj_t = typename decltype(vo.value_)::template type_at<i>;
            auto& obj   = vo.value_.template get<obj_t>();
            auto  dmo   = fc::variant();

            fc::to_variant(obj, dmo);
            mo["data"] = dmo;
        }
    });

    var = std::move(mo);
}

template<typename ENUM, typename... ARGS>
void
from_variant(const variant& var, variant_wrapper<ENUM, ARGS...>& vo) {
    using namespace boost;

    auto type = (int)var["type"].as<ENUM>();
    FC_ASSERT(type <= (int)ENUM::max_value, "Type index is not valid");

    auto range = hana::range_c<int, 0, (int)ENUM::max_value + 1>;
    hana::for_each(range, [&](auto i) {
        if(type == i()) {
            using obj_t = typename decltype(vo.value_)::template type_at<i>;
            auto  obj   = obj_t{};
            auto& vobj  = var.get_object();
            if(vobj.find("data") != vobj.end()) {
                fc::from_variant(var["data"], obj);
            }
            vo.value_ = obj;
        }
    });
}

}  // namespace fc
