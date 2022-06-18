/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <algorithm>
#include <chrono>
#include <sstream>
#include <boost/noncopyable.hpp>
#include <fmt/format.h>
#include <fc/utility.hpp>
#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/execution_context_impl.hpp>
#include <jmzk/chain/transaction_context.hpp>

namespace chainbase {
class database;
}

namespace jmzk { namespace chain {

struct action_trace;
class controller;
class transaction_context;

class apply_context : boost::noncopyable {
public:
    apply_context(controller& con, transaction_context& trx_ctx, const action& action)
        : control(con)
        , exec_ctx(trx_ctx.exec_ctx)
        , db(con.db())
        , token_db(con.token_db())
        , token_db_cache(con.token_db_cache())
        , trx_context(trx_ctx)
        , act(action) {}

public:
    void exec(action_trace& trace);
    void exec_one(action_trace& trace);

public:
    uint64_t next_global_sequence();

    bool has_authorized(const domain_name& domain, const domain_key& key) const;
    void finalize_trace(action_trace& trace, const std::chrono::steady_clock::time_point& start);
    
    uint32_t get_index_of_trx() const { return (uint32_t)trx_context.executed.size(); }

    action&
    add_generated_action(action&& act) {
        return _generated_actions.emplace_back(std::move(act));
    }

    ft_holder&
    add_new_ft_holder(ft_holder&& nfth) {
        return _new_ft_holders.emplace_back(std::move(nfth));
    }

public:
    fmt::memory_buffer&
    get_console_buffer() {
        return _pending_console_output;
    }

public:
    controller&             control;
    jmzk_execution_context&  exec_ctx;
    chainbase::database&    db;
    token_database&         token_db;
    token_database_cache&   token_db_cache;
    transaction_context&    trx_context;
    const action&           act;

private:
    fmt::memory_buffer         _pending_console_output;
    small_vector<action, 2>    _generated_actions;
    small_vector<ft_holder, 2> _new_ft_holders;
};

}}  // namespace jmzk::chain
