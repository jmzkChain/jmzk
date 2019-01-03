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
    void init(uint64_t initial_net_usage);

public:
    transaction_context(controller&            c,
                        transaction_metadata&  t,
                        fc::time_point         start = fc::time_point::now());

    void init_for_implicit_trx();
    void init_for_input_trx(bool skip_recording);
    void init_for_suspend_trx();

    void exec();
    void finalize();
    void squash();
    void undo();

    inline void add_net_usage( uint64_t u ) { net_usage += u; check_net_usage(); }

private:
    friend struct controller_impl;
    friend class apply_context;

    void dispatch_action(action_trace& trace, const action& a);
    void record_transaction(const transaction_id_type& id, fc::time_point_sec expire);

    void check_time() const;
    void check_charge();
    void check_paid() const;
    void check_net_usage() const;

    void finalize_pay();

public:
    controller& control;
    
    optional<chainbase::database::session> undo_session;
    optional<token_database::session>      undo_token_session;

    transaction_metadata& trx;
    transaction_trace_ptr trace;
    fc::time_point        start;

    small_vector<action_receipt, 4> executed;

    bool      is_input  = false;
    uint32_t  charge    = 0;
    uint64_t  net_limit = 0;
    uint64_t& net_usage;  // reference to trace->net_usage

    fc::time_point deadline = fc::time_point::maximum();

private:
    bool is_initialized = false;
};

}}  // namespace evt::chain
