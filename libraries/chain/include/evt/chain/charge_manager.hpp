/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <math.h>
#include <tuple>

#include <evt/chain/action.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/global_property_object.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/execution/execution_context.hpp>

namespace evt { namespace chain {

using act_charge_result = std::tuple<uint32_t, uint32_t>;

namespace __internal {

struct base_act_charge {
    static uint32_t
    storage(const action& act) {
        return act.data.size();
    }

    static uint32_t
    cpu(const action& act) {
        return 15;
    }

    static uint32_t
    extra_factor(const action& act) {
        return 10;
    }
};

template<uint64_t N, typename T>
struct act_charge : public base_act_charge {};

template<uint64_t N>
struct get_act_charge {
    template<typename T>
    static act_charge_result
    invoke(const action& act, const chain_config& config) {
        using charge = act_charge<N, T>;
        uint32_t s = 0;

        s += charge::storage(act) * config.base_storage_charge_factor;
        s += charge::cpu(act) * config.base_cpu_charge_factor;

        return std::make_tuple(s, charge::extra_factor(act));
    }
};

}  // namespace __internal

class charge_manager {
public:
    charge_manager(const controller& control)
        : control_(control)
        , config_(control.get_global_properties().configuration) {}

private:
    uint32_t
    network(const packed_transaction& ptrx, size_t sig_num) const {
        uint32_t s = 0;

        s += ptrx.packed_trx.size();
        s += sig_num * sizeof(signature_type);
        s += config::fixed_net_overhead_of_packed_trx;

        return s;
    }

    uint32_t
    cpu(const packed_transaction&, size_t sig_num) const {
        return sig_num * 60;
    }

public:
    uint32_t
    calculate(const packed_transaction& ptrx, size_t sig_num = 0) const {
        using namespace __internal;
        EVT_ASSERT(!ptrx.get_transaction().actions.empty(), tx_no_action, "There's not any actions in this transaction");

        sig_num = std::max(sig_num, ptrx.signatures.size());

        uint32_t ts = 0, s = 0;
        ts += network(ptrx, sig_num) * config_.base_network_charge_factor;
        ts += cpu(ptrx, sig_num) * config_.base_cpu_charge_factor;

        const auto& trx = ptrx.get_transaction();
        auto        pts = ts / trx.actions.size();
        for(auto& act : trx.actions) {
            auto as = control_.execution_context().invoke<get_act_charge, act_charge_result>(act.index_, act, config_);
            s += (std::get<0>(as) + pts) * std::get<1>(as);  // std::get<1>(as): extra factor per action
        }

        s *= config_.global_charge_factor;

#ifdef MAINNET_BUILD
        if(control_.head_block_num() >= 2750000) {
            s /= 1000'000;
        }
#else
        if(control_.head_block_num() >= 100) {
            s /= 1000'000;
        }
#endif
        return s;
    }

private:
    const controller&   control_;
    const chain_config& config_;
};

namespace __internal {

using namespace contracts;

template<typename T>
struct act_charge<N(issuetoken), T> : public base_act_charge {
    static uint32_t
    cpu(const action& act) {
        auto& itact = act.data_as<execution::add_clr_t<T>>();
        if(itact.names.empty()) {
            return 15;
        }
        return 15 + (itact.names.size() - 1) * 3;
    }
};

template<typename T>
struct act_charge<N(addmeta), T> : public base_act_charge {
    static uint32_t
    cpu(const action& act) {
        return 600;
    }

    static uint32_t
    extra_factor(const action& act) {
        return 10;
    }
};

template<typename T>
struct act_charge<N(issuefungible), T> : public base_act_charge {
    static uint32_t
    extra_factor(const action& act) {
        auto& ifact = act.data_as<execution::add_clr_t<T>>();
        auto sym = ifact.number.sym();
        // set charge to zero when issuing EVT
        if(sym == evt_sym()) {
            return 0;
        }
        return 1;
    }
};

}  // namespace __internal

}}  // namespace evt::chain
