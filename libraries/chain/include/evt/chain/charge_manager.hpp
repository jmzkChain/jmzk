/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <math.h>
#include <tuple>
#include <evt/chain/transaction.hpp>
#include <evt/chain/action.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/types_invoker.hpp>

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
        return 1;
    }

    static uint32_t
    extra_factor(const action& act) {
        return 1;
    }
};

template<typename T>
struct act_charge : public base_act_charge {};

template<typename T>
struct get_act_charge {
    static act_charge_result
    invoke(const action& act, const chain_config& config) {
        FC_ASSERT(act.name == T::get_name());

        using charge = act_charge<T>;
        uint32_t s = 0;

        s += charge::storage(act) * config.base_storage_charge_factor;
        s += charge::cpu(act) * config.base_cpu_charge_factor;

        return std::make_tuple(s, charge::extra_factor(act));
    }
};

}  // namespace __internal

class charge_manager {
public:
    charge_manager(const chain_config& config)
        : config_(config) {}

private:
    uint32_t
    network(const transaction_metadata& trx) {
        auto&    ptrx = trx.packed_trx;
        uint32_t s    = 0;

        s += ptrx.packed_trx.size();
        s += ptrx.signatures.size() * sizeof(signature_type);
        s += config::fixed_net_overhead_of_packed_trx;

        return s;
    }

    uint32_t
    cpu(const transaction_metadata& trx) {
        auto& ptrx = trx.packed_trx;
        return ptrx.signatures.size();
    }

public:
    uint32_t
    calculate(const transaction_metadata& trx) {
        using namespace __internal;

        uint32_t ts = 0, s = 0;
        ts += network(trx) * config_.base_network_charge_factor;
        ts += cpu(trx) * config_.base_cpu_charge_factor;

        auto pts = ts / trx.trx.actions.size();
        for(auto& act : trx.trx.actions) {
            auto as = types_invoker<act_charge_result, get_act_charge>::invoke(act.name, act, config_);
            s += (std::get<0>(as) + pts) * std::get<1>(as);
        }
        return s;
    }

private:
    const chain_config& config_;
};

namespace __internal {

using namespace contracts;

template<>
struct act_charge<addmeta> : public base_act_charge {
    static uint32_t
    extra_factor(const action& act) {
        return 100;
    }
};

template<>
struct act_charge<issuefungible> : public base_act_charge {
    static uint32_t
    extra_factor(const action& act) {
        auto& ifact = act.data_as<const issuefungible&>();
        auto sym = ifact.number.get_symbol();
        // set charge to zero when issuing EVT
        if(sym == symbol(SY(5,EVT))) {
            return 0;
        }
        return 1;
    }
};

}  // namespace __internal

}}  // namespace evt::chain
