/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <boost/hana/at.hpp>
#include <boost/hana/at_key.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/tuple.hpp>

namespace jmzk { namespace chain { namespace contracts {

enum class reserved_meta_key {
    disable_destroy = 0,
    disable_set_transfer
};

template<uint128_t i>
using uint128 = hana::integral_constant<uint128_t, i>;

template<uint128_t i>
constexpr uint128<i> uint128_c{};

auto domain_metas = hana::make_map(
    hana::make_pair(
        hana::int_c<(int)reserved_meta_key::disable_destroy>,
        hana::make_tuple(uint128_c<N128(.disable-destroy)>,
        hana::type_c<bool>)
    ),
    hana::make_pair(
        hana::int_c<(int)reserved_meta_key::disable_set_transfer>,
        hana::make_tuple(uint128_c<N128(.disable-set-transfer)>,
        hana::type_c<bool>)
    )
);

auto fungible_metas = hana::make_map(
    hana::make_pair(
        hana::int_c<(int)reserved_meta_key::disable_set_transfer>,
        hana::make_tuple(uint128_c<N128(.disable-set-transfer)>,
        hana::type_c<bool>)
    )
);

template<reserved_meta_key KeyType>
constexpr auto get_metakey = [](auto& metas) {
    return name128(hana::at(hana::at_key(metas, hana::int_c<(int)KeyType>), hana::int_c<0>)());
};

auto get_metavalue = [](const auto& obj, auto k) {
    for(const auto& p : obj.metas) {
        if(p.key.value == k) {
            return optional<std::string>{ p.value };
        }
    }
    return optional<std::string>();
};

auto check_reserved_meta = [](const auto& act, const auto& metas) {
    bool result = false;
    hana::for_each(hana::values(metas), [&](const auto& m) {
        if(act.key.value == hana::at(m, hana::int_c<0>)) {
            if(hana::at(m, hana::int_c<1>) == hana::type_c<bool>) {
                if(act.value == "true" || act.value == "false") {
                    result = true;
                }
                else {
                    jmzk_THROW(meta_value_exception, "Meta-Value is not valid for `bool` type");
                }
            }
        }
    });
    return result;
};

}}}  // namespace jmzk::chain::contracts
