/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/chain/execution_context_impl.hpp>
#include <jmzk/chain/trace.hpp>
#include <jmzk/chain/token_database.hpp>

namespace jmzk { namespace chain {

class controller;
class transaction_metadata;
using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;

class transaction_context {
private:
    void init(uint64_t initial_net_usage);

public:
    transaction_context(controller&                    control,
                        jmzk_execution_context&         exec_ctx,
                        const transaction_metadata_ptr trx_meta,
                        fc::time_point                 start = fc::time_point::now());

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
    controller&            control;
    jmzk_execution_context& exec_ctx;
    
    optional<chainbase::database::session> undo_session;
    optional<token_database::session>      undo_token_session;

    const transaction_metadata_ptr trx_meta;
    const signed_transaction&      trx;

    transaction_trace_ptr trace;
    fc::time_point        start;

    small_vector<action_receipt, 4> executed;

    bool      is_input    = false;
    bool      is_implicit = false;
    uint32_t  charge      = 0;
    uint64_t  net_limit   = 0;
    uint64_t& net_usage;  // reference to trace->net_usage

    fc::time_point deadline = fc::time_point::maximum();

private:
    bool is_initialized = false;
};

}}  // namespace jmzk::chain
