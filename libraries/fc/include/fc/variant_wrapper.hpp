/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
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
        return std::get<X>(value_);
    }

    template<typename X>
    const X&
    get() const {
        return std::get<X>(value_);
    }

    template<ENUM I>
    auto&
    get() {
        return std::get<(size_t)I>(value_);
    }

    template<ENUM I>
    const auto&
    get() const {
        return std::get<(size_t)I>(value_);
    }

    ENUM
    type() const {
        return (ENUM)value_.index();
    }

    template<typename Visitor>
    void
    visit(Visitor&& visitor) const {
        std::visit(std::forward<Visitor>(visitor), value_);
    }

public:
    std::variant<ARGS...> value_;
};

template<typename ENUM, typename... ARGS>
void
to_variant(const variant_wrapper<ENUM, ARGS...>& vo, variant& var) {
    using namespace boost;

    FC_ASSERT(vo.type() <= ENUM::max_value, "Invalid type index state");
    auto mo = mutable_variant_object();
    mo["type"] = vo.type();

    std::visit([&mo](auto& obj) {
        auto var = fc::variant();
        fc::to_variant(obj, var);
        mo["data"] = var;
    }, vo.value_);

    var = std::move(mo);
}

template<typename ENUM, typename... ARGS>
void
from_variant(const variant& var, variant_wrapper<ENUM, ARGS...>& vo) {
    using namespace boost;

    auto type = var["type"].as<ENUM>();
    FC_ASSERT(type <= ENUM::max_value, "Invalid type index state");

    auto range = hana::range_c<int, 0, (int)ENUM::max_value + 1>;
    hana::for_each(range, [&](auto i) {
        if((int)type == i()) {
            using obj_t = std::variant_alternative_t<i(), decltype(vo.value_)>;
            auto  obj   = obj_t{};

            fc::from_variant(var["data"], obj);
            vo.value_.template emplace<obj_t>(std::move(obj));
        }
    });
}

}  // namespace fc
