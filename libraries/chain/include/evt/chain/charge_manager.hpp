/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <math.h>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/action.hpp>
#include <evt/chain/config.hpp>

namespace evt { namespace chain {

using namespace contracts;

template<typename T>
struct base_act_charge {
    static size_t
    storage(const action& act) {
        return act.data.size();
    }

    static size_t
    cpu(const action& act) {
        return 1;
    }
};

template<typename T>
struct act_charge : public base_act_charge<T> {
    static size_t factor() { return 1; }
};

template<>
struct act_charge<addmeta> {
    static size_t factor() { return 10; }
};

template<typename T>
struct get_act_charge {
    static size_t
    operator(const action& act) {
        FC_ASSERT(act.name == T::get_name());
        auto charge = act_charge<T>();
        size_t s = 0;
        s += charge.network(act);
        s += charge.storage(act);
        s += charge.cpu(act);
        return s * charge.factor();
    }
};

struct get_trx_charge {
    static size_t
    network(const transaction_metadata& trx) {
        auto& ptrx = trx.packed_trx;

        size_t s = 0;
        s += ptrx.packed_trx.size();
        s += ptrx.signatures.size() * sizeof(signature_type);
        s += fixed_net_overhead_of_packed_trx;
        return s;
    }

    static size_t
    operator(const transaction_metadata& trx) {
        size_t s = 0;
        s += network(ptrx) * network_factor;
        for(auto& act : trx.trx.actions) {
            s += act_charge<
        }
    }
};



}}  // namespace evt::chain
