/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <type_traits>

#include <boost/hana.hpp>

#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/contracts/types.hpp>

namespace hana = boost::hana;

namespace evt { namespace chain {

namespace execution {

template<typename ... ACTTYPE>
struct execution_context {
public:
    execution_context()
        : curr_vers_() {
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
    index_of(name act) const {
        auto& arr = act_names_arr_;
        auto it   = std::lower_bound(std::cbegin(arr), std::cend(arr), act.value);
        EVT_ASSERT(it != std::cend(arr) && *it == act.value, unknown_action_exception, "Unknown action: ${act}", ("act", act));

        return std::distance(std::cbegin(arr), it);
    }

    template<typename T>
    constexpr int
    index_of() const {
        auto i = hana::index_if(act_names_, hana::equal.to(hana::ulong_c<T::get_action_name().value>));
        static_assert(i != hana::nothing, "T is not valid action type");

        return i.value()();
    }

    int
    set_version(name act, int newver) {
        auto fn = [&](auto i) -> int {
            auto name  = act_names_[i];
            auto vers  = hana::filter(act_types_,
                [&](auto& t) { return hana::equal(name, hana::ulong_c<decltype(+t)::type::get_action_name().value>); });
            auto cver = curr_vers_[i];

            EVT_ASSERT(newver > cver && newver <= (int)hana::length(vers)(), action_version_exception, "New version should be in range (${c},${m}]", ("c",cver)("m",hana::length(vers)()));

            auto old_ver  = curr_vers_[i];
            curr_vers_[i] = newver;

            return old_ver;
        };

        auto actindex = index_of(act);
        auto range    = hana::make_range(hana::int_c<0>, hana::length(act_names_));
        auto old_ver  = 0;
        hana::for_each(range, [&](auto i) {
            if(i() == actindex) {
                old_ver = fn(i);
            }
        });

        return old_ver;
    }

    name
    get_acttype_name(int index) const {
        return type_names_[index][get_curr_ver(index)];
    }

    template <template<uint64_t> typename Invoker, typename RType, typename ... Args>
    RType
    invoke(int actindex, Args&&... args) const {
        using opt_type = std::conditional_t<std::is_void<RType>::value, int, RType>;

        auto fn = [&](auto i) -> std::optional<opt_type> {
            auto name  = act_names_[i];
            auto vers  = hana::filter(act_types_,
                [&](auto& t) { return hana::equal(name, hana::ulong_c<decltype(+t)::type::get_action_name().value>); });
            auto cver = curr_vers_[i];

            static_assert(hana::length(vers)() > hana::size_c<0>(), "empty version actions!");

            auto result = std::optional<opt_type>();
            hana::for_each(vers, [&, cver](auto v) {
                using ty = typename decltype(+v)::type;

                constexpr auto n = ty::get_action_name().value;
                if(ty::get_version() == cver) {
                    if constexpr (std::is_void<RType>::value) {
                        Invoker<n>::template invoke<ty>(std::forward<Args>(args)...);
                        result = 1;
                    }
                    else {
                        result = Invoker<n>::template invoke<ty>(std::forward<Args>(args)...);
                    }
                }
            });

            return result;
        };

        auto range  = hana::make_range(hana::int_c<0>, hana::length(act_names_));
        auto result = std::optional<opt_type>();
        hana::for_each(range, [&](auto i) {
            if(i() == actindex) {
                result = fn(i);
            }
        });

        EVT_ASSERT(result.has_value(), action_index_exception, "Invalid action index: ${act}", ("act", actindex));
        if constexpr (!std::is_void<RType>::value) {
            return *result;
        }
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
};

using execution_context_impl = execution_context<
                                   contracts::newdomain,
                                   contracts::updatedomain,
                                   contracts::issuetoken,
                                   contracts::transfer,
                                   contracts::destroytoken,
                                   contracts::newgroup,
                                   contracts::updategroup,
                                   contracts::newfungible,
                                   contracts::updfungible,
                                   contracts::issuefungible,
                                   contracts::transferft,
                                   contracts::recycleft,
                                   contracts::destroyft,
                                   contracts::evt2pevt,
                                   contracts::addmeta,
                                   contracts::newsuspend,
                                   contracts::cancelsuspend,
                                   contracts::aprvsuspend,
                                   contracts::execsuspend,
                                   contracts::paycharge,
                                   contracts::everipass,
                                   contracts::everipay,
                                   contracts::prodvote,
                                   contracts::updsched,
                                   contracts::newlock,
                                   contracts::aprvlock,
                                   contracts::tryunlock
                               >;

// helper method to add const lvalue reference to type object
template<typename T>
using add_clr_t = typename std::add_const<typename std::add_lvalue_reference<T>::type>::type;

}}}  // namespace evt::chain::execution
