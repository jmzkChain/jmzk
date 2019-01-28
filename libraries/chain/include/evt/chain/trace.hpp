/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/action.hpp>
#include <evt/chain/action_receipt.hpp>
#include <evt/chain/block.hpp>

namespace evt { namespace chain {

struct new_account {
    address        addr;
    symbol_id_type sym_id;
};

struct action_trace {
    action_trace(const action_receipt& r)
        : receipt(r) {}
    action_trace() {}

    action_receipt   receipt;
    action           act;
    fc::microseconds elapsed;
    string           console;

    transaction_id_type  trx_id; ///< the transaction that generated this action
    uint32_t             block_num = 0;
    block_timestamp_type block_time;

    std::optional<block_id_type> producer_block_id;
    std::optional<fc::exception> except;

    small_vector<action, 2>      generated_actions;
    small_vector<new_account, 2> new_accounts;
};

struct transaction_trace;
using transaction_trace_ptr = std::shared_ptr<transaction_trace>;

struct transaction_trace {
    transaction_id_type                       id;
    std::optional<transaction_receipt_header> receipt;
    fc::microseconds                          elapsed;
    bool                                      is_suspend = false;
    vector<action_trace>                      action_traces;  ///< disposable

    uint32_t charge;
    uint64_t net_usage;

    std::optional<fc::exception> except;
    std::exception_ptr           except_ptr;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::action_trace, (receipt)(act)(elapsed)(console)(trx_id)(block_num)(block_time)(producer_block_id)(except)(generated_actions));
FC_REFLECT(evt::chain::transaction_trace, (id)(receipt)(elapsed)(is_suspend)(action_traces)(charge)(net_usage)(except));
