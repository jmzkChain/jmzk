/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <algorithm>
#include <chrono>
#include <sstream>
#include <boost/noncopyable.hpp>
#include <fmt/format.h>
#include <fc/utility.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/execution_context_impl.hpp>
#include <evt/chain/transaction_context.hpp>

namespace chainbase {
class database;
}

namespace evt { namespace chain {

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
        for(auto& fth : _new_ft_holders) {
            if(fth == nfth) {
                return fth;
            }
        }
        return _new_ft_holders.emplace_back(std::move(nfth));
    }

public:
    fmt::memory_buffer&
    get_console_buffer() {
        return _pending_console_output;
    }

public:
    controller&             control;
    evt_execution_context&  exec_ctx;
    chainbase::database&    db;
    token_database&         token_db;
    transaction_context&    trx_context;
    const action&           act;

private:
    fmt::memory_buffer         _pending_console_output;
    small_vector<action, 2>    _generated_actions;
    small_vector<ft_holder, 2> _new_ft_holders;
};

}}  // namespace evt::chain
