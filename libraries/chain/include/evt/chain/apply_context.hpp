/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <algorithm>
#include <sstream>
#include <boost/noncopyable.hpp>
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
    
    void finalize_trace( action_trace& trace, const fc::time_point& start );

public:
    void reset_console();

    std::ostringstream&
    get_console_stream() {
        return _pending_console_output;
    }

    const std::ostringstream&
    get_console_stream() const {
        return _pending_console_output;
    }

    template <typename T>
    void
    console_append(T val) {
        _pending_console_output << val;
    }

    template <typename T, typename... Ts>
    void
    console_append(T val, Ts... rest) {
        console_append(val);
        console_append(rest...);
    };

    inline void
    console_append_formatted(const string& fmt, const variant_object& vo) {
        console_append(fc::format_string(fmt, vo));
    }

public:
    controller&          control;
    chainbase::database& db;
    token_database&      token_db;
    transaction_context& trx_context;
    const action&        act;

private:
    std::ostringstream _pending_console_output;
};

}}  // namespace evt::chain
