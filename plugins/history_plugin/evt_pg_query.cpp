/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/history_plugin/evt_pg_query.hpp>

#include <functional>
#include <fmt/format.h>
#include <libpq-fe.h>
#include <boost/lexical_cast.hpp>
#include <fc/io/json.hpp>
#include <evt/chain/block_header.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/http_plugin/http_plugin.hpp>

namespace evt {

namespace __internal {

struct __prepare_register {
public:
    std::map<std::string, std::string> stmts;

public:
    static __prepare_register& instance() {
        static __prepare_register i;
        return i;
    }
};

struct __insert_prepare {
    __insert_prepare(const std::string& name, const std::string sql) {
        __prepare_register::instance().stmts[name] = sql;
    }
};

#define PREPARE_SQL_ONCE(name, sql) \
    __internal::__insert_prepare __##name(#name, sql);

template<typename Iterator>
void
format_array_to(fmt::memory_buffer& buf, Iterator begin, Iterator end) {
    fmt::format_to(buf, fmt("{{"));
    if(begin != end) {
        auto it = begin;
        for(; it != end - 1; it++) {
            fmt::format_to(buf, fmt("\"{}\","), (std::string)*it);
        }
        fmt::format_to(buf, fmt("\"{}\""), (std::string)*it);
    }
    fmt::format_to(buf, fmt("}}\t"));
}

enum task_type {
    kGetTokens = 0,
    kGetDomains,
    kGetGroups,
    kGetFungibles,
    kGetActions
};

template<typename T>
int
response_ok(int id, const T& obj) {
    app().get_plugin<http_plugin>().set_deferred_response(id, 200, fc::json::to_string(obj));
    return PG_OK;
}

template<>
int
response_ok<std::string>(int id, const std::string& json) {
    app().get_plugin<http_plugin>().set_deferred_response(id, 200, json);
    return PG_OK;
}

}  // namespace __internal

int
pg_query::connect(const std::string& conn) {
    conn_ = PQconnectdb(conn.c_str());

    auto status = PQstatus(conn_);
    EVT_ASSERT(status == CONNECTION_OK, chain::postgres_connection_exception, "Connect failed");

    socket_ = boost::asio::ip::tcp::socket(io_serv_, boost::asio::ip::tcp::v4(), PQsocket(conn_));
    return PG_OK;
}

int
pg_query::close() {
    FC_ASSERT(conn_);
    PQfinish(conn_);
    conn_ = nullptr;

    return PG_OK;
}

int
pg_query::prepare_stmts() {
    for(auto it : __internal::__prepare_register::instance().stmts) {
        auto r = PQprepare(conn_, it.first.c_str(), it.second.c_str(), 0, NULL);
        EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgres_exec_exception,
            "Prepare sql failed, sql: ${s}, detail: ${d}", ("s",it.second)("d",PQerrorMessage(conn_)));
        PQclear(r);
    }
    return PG_OK;
}

int
pg_query::begin_poll_read() {
    socket_.async_wait(boost::asio::ip::tcp::socket::wait_type::wait_read, std::bind(&pg_query::poll_read, this));
    return PG_OK;
}

int
pg_query::queue(int id, int task) {
    tasks_.emplace(id, task);
    return PG_OK;
}

int
pg_query::poll_read() {
    using namespace __internal;

    while(1) {
        auto r = PQconsumeInput(conn_);
        EVT_ASSERT(r, chain::postgres_poll_exception, "Poll messages from postgres failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

        if(PQisBusy(conn_)) {
            break;
        }

        auto re = PQgetResult(conn_);
        if(re == NULL) {
            break;
        }

        auto t = tasks_.front();
        tasks_.pop();

        switch(t.type) {
        case kGetTokens: {
            get_tokens_resume(t.id, re);
            break;
        }
        case kGetDomains: {
            get_domains_resume(t.id, re);
            break;
        }
        case kGetGroups: {
            get_groups_resume(t.id, re);
            break;
        }
        case kGetFungibles: {
            get_fungibles_resume(t.id, re);
            break;
        }
        case kGetActions: {
            get_actions_resume(t.id, re);
            break;
        }
        };  // switch

        PQclear(re);
    }

    socket_.async_wait(boost::asio::ip::tcp::socket::wait_type::wait_read, std::bind(&pg_query::poll_read, this));
    return PG_OK;
}

PREPARE_SQL_ONCE(gt_plan,  "SELECT domain, name FROM tokens WHERE $1 @> owner AND domain = $2");
PREPARE_SQL_ONCE(gt_plan2, "SELECT domain, name FROM tokens WHERE $1 @> owner");  // without domain filter

int
pg_query::get_tokens_async(int id, const read_only::get_tokens_params& params) {
    using namespace __internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = std::string();
    if(params.domain.valid()) {
        stmt = fmt::format(fmt("EXECUTE gt_plan ('{}','{}');"), fmt::to_string(pkeys_buf), (std::string)*params.domain);
    }
    else {
        stmt = fmt::format(fmt("EXECUTE gt_plan2 ('{}');"), fmt::to_string(pkeys_buf));
    }

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get tokens command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetTokens);
}

int
pg_query::get_tokens_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get tokens failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto results = fc::mutable_variant_object();
    for(int i = 0; i < PQntuples(r); i++) {
        auto domain = PQgetvalue(r, i, 0);
        auto name   = PQgetvalue(r, i, 1);

        if(results.find(domain) == results.end()) {
            results.set(domain, fc::variants());
        }
        results[domain].get_array().emplace_back(name);
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gd_plan, "SELECT name FROM domains WHERE creator = ANY($1);");

int
pg_query::get_domains_async(int id, const read_only::get_params& params) {
    using namespace __internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gd_plan ('{}')"), fmt::to_string(pkeys_buf));

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get domains command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetDomains);
}

int
pg_query::get_domains_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get domains failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto results = std::vector<const char*>();
    for(int i = 0; i < PQntuples(r); i++) {
        auto name = PQgetvalue(r, i, 0);
        results.emplace_back(name);
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gg_plan, "SELECT name FROM groups WHERE key = ANY($1);");

int
pg_query::get_groups_async(int id, const read_only::get_params& params) {
    using namespace __internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gg_plan ('{}')"), fmt::to_string(pkeys_buf));

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get groups command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetGroups);
}

int
pg_query::get_groups_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get groups failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto results = std::vector<const char*>();
    for(int i = 0; i < PQntuples(r); i++) {
        auto name = PQgetvalue(r, i, 0);
        results.emplace_back(name);
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gf_plan, "SELECT sym_id FROM fungibles WHERE creator = ANY($1);");

int
pg_query::get_fungibles_async(int id, const read_only::get_params& params) {
    using namespace __internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gf_plan ('{}')"), fmt::to_string(pkeys_buf));

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get fungibles command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetFungibles);
}

int
pg_query::get_fungibles_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungibles failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto results = std::vector<int64_t>();
    for(int i = 0; i < PQntuples(r); i++) {
        auto sym_id = PQgetvalue(r, i, 0);
        results.emplace_back(boost::lexical_cast<int64_t>(sym_id));
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(ga_plan, R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                                FROM actions
                                JOIN blocks ON actions.block_id = blocks.block_id
                                WHERE domain = $1
                                ORDER BY actions.created_at $2 actions.seq_num $2
                                LIMIT $3 OFFSET $4
                                )sql");

// with key filter
PREPARE_SQL_ONCE(ga_plan2, R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                                 FROM actions
                                 JOIN blocks ON actions.block_id = blocks.block_id
                                 WHERE domain = $1 AND key = $2
                                 ORDER BY actions.created_at $3 actions.seq_num $3
                                 LIMIT $4 OFFSET $5
                                 )sql");


// with name filter
PREPARE_SQL_ONCE(ga_plan3, R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                                 FROM actions
                                 JOIN blocks ON actions.block_id = blocks.block_id
                                 WHERE domain = $1 AND name = ANY($2)
                                 ORDER BY actions.created_at $3 actions.seq_num $3
                                 LIMIT $4 OFFSET $5
                                 )sql");


// with key and name filter
PREPARE_SQL_ONCE(ga_plan4, R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                                 FROM actions
                                 JOIN blocks ON actions.block_id = blocks.block_id
                                 WHERE domain = $1 AND key = $2 AND name = ANY($3)
                                 ORDER BY actions.created_at $4 actions.seq_num $4
                                 LIMIT $5 OFFSET $6
                                 )sql");

int
pg_query::get_actions_async(int id, const read_only::get_actions_params& params) {
    using namespace __internal;

    int s = 0, t = 10;
    if(params.skip.valid()) {
        s = *params.skip;
    }
    if(params.take.valid()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::postgres_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto dire = (params.dire.valid() && *params.dire == direction::asc) ? "ASC" : "DESC";
    auto stmt = std::string();

    int j = 0;
    if(params.key.valid()) {
        j += 1;
    }
    if(!params.names.empty()) {
        j += 2;
    }

    switch(j) {
    case 0 : { // only domain
        stmt = fmt::format(fmt("EXECUTE ga_plan ('{}',{},{},{});"), (std::string)params.domain, dire, t, s);
        break;
    }
    case 1: { // domain + key
        stmt = fmt::format(fmt("EXECUTE ga_plan2 ('{}','{}',{},{},{});"), (std::string)params.domain, (std::string)*params.key, dire, t, s);
        break;
    }
    case 2: { // domain + name
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan3 ('{}','{}',{},{},{});"), (std::string)params.domain, fmt::to_string(names_buf), dire, t, s);
        break;
    }
    case 3: { // domain + key + name
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan4 ('{}','{}','{}',{},{},{});"), (std::string)params.domain, (std::string)*params.key, fmt::to_string(names_buf), dire, t, s);
        break;
    }
    };  // switch

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get fungibles command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetActions);
}

int
pg_query::get_actions_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get actions failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto builder = fmt::memory_buffer();
    auto n       = PQntuples(r);

    fmt::format_to(builder, "[");
    for(int i = 0; i < n; i++) {
        fmt::format_to(builder,
            fmt(R"({{"trx_id":"{}","name":"{}","domain":"{}","key":"{}","data":{},"timestamp":"{}"}})"),
            PQgetvalue(r, i, 0),
            PQgetvalue(r, i, 1),
            PQgetvalue(r, i, 2),
            PQgetvalue(r, i, 3),
            PQgetvalue(r, i, 4),
            PQgetvalue(r, i, 5)
            );
        if(i < n - 1) {
            fmt::format_to(builder, ",");
        }
    }
    fmt::format_to(builder, "]");

    return response_ok(id, fmt::to_string(builder));
}

}  // namepsace evt