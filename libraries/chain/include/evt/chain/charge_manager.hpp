/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <math.h>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/action.hpp>

namespace evt { namespace chain {

using namespace contracts;

template<typename T>
struct base_charge {
    static size_t
    network(const signed_transaction& trx, const action& act) {
        size_t s = sizeof(act.domain) + sizeof(act.key) + sizeof(act.name);
        s += act.data.size();
        s += ::round(sizeof(signature_type) * trx.signatures.size() / trx.total_actions());
        return s;
    }

    static size_t
    storage(const signed_transaction& trx, const action& act) {
        return act.data.size();
    }

    static size_t
    code_complexity() {
        return 1;
    }

    static size_t
    cpu(const signed_transaction& trx, const action& act) {
        size_t s = code_complexity();
        s += trx.signatures.size() / trx.total_actions();
    }
};

template<typename T>
struct charge : public base_charge<T> {
    static size_t factor() { return 1; }
};

template<>
struct charge<addmeta> {
    static size_t factor() { return 10; }
};

template<typename T>
struct get_charge {
    static size_t
    operator(const action& act) {
        FC_ASSERT(act.name == T::get_name());
        auto charge = act_charge<T>();
        return charge.storage(act) * charge.factor();
    }
};



}}  // namespace evt::chain
