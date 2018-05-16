/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/controller.hpp>
#include <evt/chain/trace.hpp>

namespace evt { namespace chain {

class transaction_context {
private:
    void init();

public:
    transaction_context(controller&                c,
                        const signed_transaction&  t,
                        const transaction_id_type& trx_id,
                        fc::time_point             start = fc::time_point::now());

    void init_for_implicit_trx();
    void init_for_input_trx(uint32_t num_signatures);

    void exec();
    void finalize();

    void checktime() const;

private:
    friend struct controller_impl;
    friend class apply_context;

    void dispatch_action(action_trace& trace, const action& a);
    void record_transaction(const transaction_id_type& id, fc::time_point_sec expire);

    /// Fields:
public:
    controller&               control;
    const signed_transaction& trx;
    transaction_id_type       id;
    transaction_trace_ptr     trace;
    fc::time_point            start;
    fc::time_point            published;

    vector<action_receipt> executed;

    bool is_input = false;

    fc::time_point   deadline = fc::time_point::maximum();
    fc::microseconds leeway   = fc::microseconds(1000);

private:
    bool is_initialized = false;
};

}}  // namespace evt::chain
