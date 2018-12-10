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

namespace chainbase {
class database;
}

namespace evt { namespace chain {

struct action_trace;
class transaction_context;

class apply_context : boost::noncopyable {
public:
    apply_context(controller& con, transaction_context& trx_ctx, const action& action)
        : control(con)
        , db(con.db())
        , token_db(con.token_db())
        , trx_context(trx_ctx)
        , act(action) {
        reset_console();
    }

public:
    void exec(action_trace& trace);
    void exec_one(action_trace& trace);

public:
    uint64_t next_global_sequence();

    bool has_authorized(const domain_name& domain, const domain_key& key) const;
    
    void finalize_trace( action_trace& trace, const std::chrono::steady_clock::time_point& start);

public:
    void reset_console();

    fmt::memory_buffer&
    get_console_buffer() {
        return _pending_console_output;
    }

public:
    controller&          control;
    chainbase::database& db;
    token_database&      token_db;
    transaction_context& trx_context;
    const action&        act;

private:
    fmt::memory_buffer _pending_console_output;
};

}}  // namespace evt::chain
