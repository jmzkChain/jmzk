/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <math.h>
#include <evt/chain/transaction.hpp>
#include <evt/chain/action.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/chain_config.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/types_invoker.hpp>

namespace evt { namespace chain {

namespace __internal {

struct base_act_charge {
    static size_t
    storage(const action& act) {
        return act.data.size();
    }

    static size_t
    cpu(const action& act) {
        return 1;
    }

    static size_t extra_factor() { return 1; }
};

template<typename T>
struct act_charge : public base_act_charge {};

template<typename T>
struct get_act_charge {
    static size_t
    invoke(const action& act, const chain_config& config) {
        FC_ASSERT(act.name == T::get_name());

        using charge = act_charge<T>;
        size_t s = 0;

        s += charge::storage(act) * config.base_storage_charge_factor;
        s += charge::cpu(act) * config.base_cpu_charge_factor;

        return s * charge::extra_factor();
    }
};

}  // namespace __internal

class charge_manager {
public:
    charge_manager(const chain_config& config)
        : config_(config) {}

private:
    size_t
    network(const transaction_metadata& trx) {
        auto&  ptrx = trx.packed_trx;
        size_t s    = 0;

        s += ptrx.packed_trx.size();
        s += ptrx.signatures.size() * sizeof(signature_type);
        s += config::fixed_net_overhead_of_packed_trx;

        return s;
    }

    size_t
    cpu(const transaction_metadata& trx) {
        auto& ptrx = trx.packed_trx;
        return ptrx.signatures.size();
    }

public:
    size_t
    calculate(const transaction_metadata& trx) {
        using namespace __internal;

        size_t s = 0;

        s += network(trx) * config_.base_network_charge_factor;
        s += cpu(trx) * config_.base_cpu_charge_factor;

        for(auto& act : trx.trx.actions) {
            s += types_invoker<size_t, get_act_charge>::invoke(act.name, act, config_);
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
    static size_t extra_factor() { return 10; }
};

}  // namespace __internal

}}  // namespace evt::chain
