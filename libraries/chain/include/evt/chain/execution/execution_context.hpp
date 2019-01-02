/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <algorithm>
#include <array>
#include <functional>

#include <boost/hana.hpp>

#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/types.hpp>

namespace hana = boost::hana;

namespace evt { namespace chain {

class authority_checker;

namespace execution {

template<typename ... ACTTYPE>
struct execution_context {
public:
    execution_context()
        : curr_vers_()
        , abi_(contracts::evt_contract_abi(), fc::milliseconds(config::default_abi_serializer_max_time_ms)) {
        constexpr auto index_of = [](auto& act) {
            return hana::index_if(act_names_, hana::equal.to(hana::ulong_c<decltype(+act)::type::get_action_name().value>)).value();
        };

        auto act_types = hana::sort.by(hana::ordering([](auto& act) { return hana::int_c<decltype(+act)::type::get_version()>; }), act_types_);
        hana::for_each(act_types, [&](auto& act) {
            auto i = index_of(act);
            FC_ASSERT(type_names_[i].size() == decltype(+act)::type::get_version());
            type_names_[i].emplace_back(decltype(+act)::type::get_type_name());
        });

        act_names_arr_ = hana::unpack(act_names_, [](auto ...i) {
            return std::array<uint64_t, sizeof...(i)>{{i...}};
        });
    }

public:
    int
    index_of(name act) {
        auto& arr = act_names_arr_;
        auto it   = std::lower_bound(std::cbegin(arr), std::cend(arr), act.value);
        EVT_ASSERT(it != std::cend(arr) && *it == act.value, action_exception, "Unknown action: ${act}", ("act", act));

        return std::distance(std::cbegin(arr), it);
    }

    fc::variant
    binary_to_variant(int actindex, const bytes& binary, bool short_path = false) const {
        auto ver     = get_curr_ver(actindex);
        auto acttype = get_acttype_name(actindex, ver);
        return abi_.binary_to_variant(acttype.to_string(), binary, short_path);
    }

    bytes
    variant_to_binary(int actindex, const fc::variant& var, bool short_path = false) const {
        auto ver     = get_curr_ver(actindex);
        auto acttype = get_acttype_name(actindex, ver);
        return abi_.variant_to_binary(acttype.to_string(), var, short_path);
    }

    template <typename RType, template<uint64_t> typename Invoker, typename ... Args>
    RType
    invoke(int actindex, Args&&... args) {
        auto fn = [&](auto i) -> std::optional<RType> {
            auto name  = act_names_[i];
            auto vers  = hana::filter(act_types_,
                [&](auto& t) { return hana::equal(name, hana::ulong_c<decltype(+t)::type::get_action_name().value>); });
            auto cver = curr_vers_[i];

            static_assert(hana::length(vers) > hana::size_c<0>, "empty version actions!");

            auto result = std::optional<RType>();
            hana::for_each(vers, [&, cver](auto& v) {
                using ty = typename decltype(+v)::type;
                if(ty::get_version() == cver) {
                    if constexpr (std::is_void<RType>::value) {     
                        Invoker<name>::invoke(hana::type_c<ty>, std::forward<Args>(args)...);
                    }
                    else {
                        result = Invoker<name>::invoke(hana::type_c<ty>, std::forward<Args>(args)...);
                    }
                }
            });

            return result;
        };

        auto range  = hana::make_range(hana::int_c<0>, hana::length(act_names_));
        auto result = std::optional<RType>();
        hana::for_each(range, [&](auto i) {
            if(i() == actindex) {
                result = fn(i);
            }
        });

        EVT_ASSERT(result.has_value(), action_exception, "Unknown action index: ${act}", ("act", actindex));
        return *result;
    }

private:
    int
    get_curr_ver(int index) const {
        return curr_vers_[index];
    }

    name
    get_acttype_name(int index, int version) const {
        return type_names_[index][version];
    }

private:
    static constexpr auto act_types_ = hana::make_tuple(hana::type_c<ACTTYPE>...);
    static constexpr auto act_names_ = hana::sort(hana::unique(hana::transform(act_types_, [](auto& a) { return hana::ulong_c<decltype(+a)::type::get_action_name().value>; })));

private:
    std::array<int, hana::length(act_names_)>                   curr_vers_;
    std::array<uint64_t, hana::length(act_names_)>              act_names_arr_;
    std::array<small_vector<name, 4>, hana::length(act_names_)> type_names_;

private:
    contracts::abi_serializer abi_;
};

using execution_context_impl = execution_context<
                                   contracts::newdomain,
                                   contracts::updatedomain,
                                   contracts::issuetoken,
                                   contracts::transfer
                               >;

}}}  // namespace evt::chain::execution
