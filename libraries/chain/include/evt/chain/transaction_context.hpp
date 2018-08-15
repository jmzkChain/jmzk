/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/controller.hpp>
#include <evt/chain/trace.hpp>
#include <evt/chain/token_database.hpp>

namespace evt { namespace chain {

class transaction_context {
private:
    void init();

public:
    transaction_context(controller&            c,
                        transaction_metadata&  t,
                        fc::time_point         start = fc::time_point::now());

    void init_for_implicit_trx();
    void init_for_input_trx(uint32_t num_signatures);
    void init_for_suspend_trx();

    void exec();
    void finalize();
    void squash();
    void undo();

private:
    friend struct controller_impl;
    friend class apply_context;

    void dispatch_action(action_trace& trace, const action& a);
    void record_transaction(const transaction_id_type& id, fc::time_point_sec expire);

    void check_time() const;
    void check_charge();
    void check_paid() const;

    void finalize_pay();

public:
    controller&             control;
    token_database::session undo_session;
    transaction_metadata&   trx;
    transaction_trace_ptr   trace;
    fc::time_point          start;

    vector<action_receipt> executed;

    bool     is_input = false;
    uint32_t charge   = 0;

    fc::time_point deadline = fc::time_point::maximum();

private:
    bool is_initialized = false;
};

}}  // namespace evt::chain
