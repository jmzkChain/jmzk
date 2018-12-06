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
    kGetActions,
    kGetFungibleActions,
    kGetTransaction,
    kGetTransactions,
    kGetFungibleIds
};

const char* call_names[] = {
    "get_tokens",
    "get_domains",
    "get_groups",
    "get_fungibles",
    "get_actions",
    "get_fungible_actions",
    "get_transaction",
    "get_transactions",
    "get_fungible_ids"
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

        try {
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
            case kGetFungibleActions: {
                get_fungible_actions_resume(t.id, re);
                break;
            }
            case kGetTransaction: {
                get_transaction_resume(t.id, re);
                break;
            }
            case kGetTransactions: {
                get_transactions_resume(t.id, re);
                break;
            }
            case kGetFungibleIds: {
                get_fungible_ids_resume(t.id, re);
                break;
            }
            };  // switch
        }
        catch(...) {
            app().get_plugin<http_plugin>().handle_async_exception(t.id, "history", call_names[t.id], "");
        }

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

auto ga_plan0 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                      FROM actions
                      JOIN blocks ON actions.block_id = blocks.block_id
                      WHERE domain = $1
                      ORDER BY actions.created_at {0}, actions.seq_num {0}
                      LIMIT $2 OFFSET $3
                      )sql";

// with key filter
auto ga_plan1 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                      FROM actions
                      JOIN blocks ON actions.block_id = blocks.block_id
                      WHERE domain = $1 AND key = $2
                      ORDER BY actions.created_at {0}, actions.seq_num {0}
                      LIMIT $3 OFFSET $4
                      )sql";

// with name filter
auto ga_plan2 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                      FROM actions
                      JOIN blocks ON actions.block_id = blocks.block_id
                      WHERE domain = $1 AND name = ANY($2)
                      ORDER BY actions.created_at {0}, actions.seq_num {0}
                      LIMIT $3 OFFSET $4
                      )sql";

// with key and name filter
auto ga_plan3 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                      FROM actions
                      JOIN blocks ON actions.block_id = blocks.block_id
                      WHERE domain = $1 AND key = $2 AND name = ANY($3)
                      ORDER BY actions.created_at {0}, actions.seq_num {0}
                      LIMIT $4 OFFSET $5
                      )sql";

PREPARE_SQL_ONCE(ga_plan01, fmt::format(ga_plan0, "DESC"));
PREPARE_SQL_ONCE(ga_plan02, fmt::format(ga_plan0, "ASC"));
PREPARE_SQL_ONCE(ga_plan11, fmt::format(ga_plan1, "DESC"));
PREPARE_SQL_ONCE(ga_plan12, fmt::format(ga_plan1, "ASC"));
PREPARE_SQL_ONCE(ga_plan21, fmt::format(ga_plan2, "DESC"));
PREPARE_SQL_ONCE(ga_plan22, fmt::format(ga_plan2, "ASC"));
PREPARE_SQL_ONCE(ga_plan31, fmt::format(ga_plan3, "DESC"));
PREPARE_SQL_ONCE(ga_plan32, fmt::format(ga_plan3, "ASC"));

int
pg_query::get_actions_async(int id, const read_only::get_actions_params& params) {
    using namespace __internal;

    int s = 0, t = 10;
    if(params.skip.valid()) {
        s = *params.skip;
    }
    if(params.take.valid()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto stmt = std::string();

    int j = 0;
    if(params.dire.valid() && *params.dire == direction::asc) {
        j += 1;
    }
    if(params.key.valid()) {
        j += 2;
    }
    if(!params.names.empty()) {
        j += 4;
    }

    switch(j) {
    case 0: { // only domain, desc
        stmt = fmt::format(fmt("EXECUTE ga_plan01 ('{}',{},{});"), (std::string)params.domain, t, s);
        break;
    }
    case 1: { // only domain, asc
        stmt = fmt::format(fmt("EXECUTE ga_plan02 ('{}',{},{});"), (std::string)params.domain, t, s);
        break;
    }
    case 2: { // domain + key, desc
        stmt = fmt::format(fmt("EXECUTE ga_plan11 ('{}','{}',{},{});"), (std::string)params.domain, (std::string)*params.key, t, s);
        break;
    }
    case 3: { // domain + key, asc
        stmt = fmt::format(fmt("EXECUTE ga_plan12 ('{}','{}',{},{});"), (std::string)params.domain, (std::string)*params.key, t, s);
        break;
    }
    case 4: { // domain + name, desc
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan21 ('{}','{}',{},{});"), (std::string)params.domain, fmt::to_string(names_buf), t, s);
        break;
    }
    case 5: { // domain + name, asc
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan22 ('{}','{}',{},{});"), (std::string)params.domain, fmt::to_string(names_buf), t, s);
        break;
    }
    case 6: { // domain + key + name, desc
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan31 ('{}','{}','{}',{},{});"), (std::string)params.domain, (std::string)*params.key, fmt::to_string(names_buf), t, s);
        break;
    }
    case 7: { // domain + key + name, asc
        auto names_buf = fmt::memory_buffer();
        format_array_to(names_buf, std::begin(params.names), std::end(params.names));

        stmt = fmt::format(fmt("EXECUTE ga_plan32 ('{}','{}','{}',{},{});"), (std::string)params.domain, (std::string)*params.key, fmt::to_string(names_buf), t, s);
        break;
    }
    };  // switch

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get actions command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

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

auto gfa_plan0 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                       FROM actions
                       JOIN blocks ON actions.block_id = blocks.block_id
                       WHERE
                           domain = '.fungible'
                           AND key = $1
                           AND name = ANY('{{"issuefungible","transferft","recycleft","evt2pevt","everipay","paycharge"}}')
                       ORDER BY actions.created_at {0}, actions.seq_num {0}
                       LIMIT $2 OFFSET $3
                       )sql";

// with address filter
auto gfa_plan1 = R"sql(SELECT trx_id, name, domain, key, data, blocks.timestamp
                       FROM actions
                       JOIN blocks ON actions.block_id = blocks.block_id
                       WHERE
                           domain = '.fungible'
                           AND key = $1
                           AND name = ANY('{{"issuefungible","transferft","recycleft","evt2pevt","everipay","paycharge"}}')
                           AND (
                               data->>'address' = $2 OR
                               data->>'from' = $2 OR
                               data->>'to' = $2 OR
                               data->>'payee' = $2 OR
                               data->'link'->'keys' @> $3 OR
                               data->>'payer' = $2
                           )
                       ORDER BY actions.created_at {0}, actions.seq_num {0}
                       LIMIT $4 OFFSET $5
                       )sql";

PREPARE_SQL_ONCE(gfa_plan01, fmt::format(gfa_plan0, "DESC"));
PREPARE_SQL_ONCE(gfa_plan02, fmt::format(gfa_plan0, "ASC"));
PREPARE_SQL_ONCE(gfa_plan11, fmt::format(gfa_plan1, "DESC"));
PREPARE_SQL_ONCE(gfa_plan12, fmt::format(gfa_plan1, "ASC"));

int
pg_query::get_fungible_actions_async(int id, const read_only::get_fungible_actions_params& params) {
    using namespace __internal;

    int s = 0, t = 10;
    if(params.skip.valid()) {
        s = *params.skip;
    }
    if(params.take.valid()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto stmt = std::string();

    int j = 0;
    if(params.dire.valid() && *params.dire == direction::asc) {
        j += 1;
    }
    if(params.addr.valid()) {
        j += 2;
    }

    switch(j) {
    case 0: { // only sym id, desc
        stmt = fmt::format(fmt("EXECUTE gfa_plan01 ('{}',{},{});"), params.sym_id, t, s);
        break;
    }
    case 1: { // only sym id, asc
        stmt = fmt::format(fmt("EXECUTE gfa_plan02 ('{}',{},{});"), params.sym_id, t, s);
        break;
    }
    case 2: { // sym id + address, desc
        stmt = fmt::format(fmt("EXECUTE gfa_plan11 ('{0}','{1}','\"{1}\"',{2},{3});"), params.sym_id, (std::string)*params.addr, t, s);
        break;
    }
    case 3: { // sym id + address, asc
        stmt = fmt::format(fmt("EXECUTE gfa_plan12 ('{0}','{1}','\"{1}\"',{2},{3});"), params.sym_id, (std::string)*params.addr, t, s);
        break;
    }
    };  // switch

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get fungible actions command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetFungibleActions);
}

int
pg_query::get_fungible_actions_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungible actions failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

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

PREPARE_SQL_ONCE(gtrx_plan, "SELECT block_num, trx_id FROM transactions WHERE trx_id = $1;");

int
pg_query::get_transaction_async(int id, const read_only::get_transaction_params& params) {
    using namespace __internal;

    auto stmt = fmt::format(fmt("EXECUTE gtrx_plan('{}');"), (std::string)params.id);

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get transaction command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetTransaction);
}

int
pg_query::get_transaction_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get transaction failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        EVT_THROW(chain::unknown_transaction_exception, "Cannot find transaction");
    }

    auto trx_id = transaction_id_type(std::string(PQgetvalue(r, 0, 1), PQgetlength(r, 0, 1)));
    for(int i = 0; i < n; i++) {
        auto block_num = boost::lexical_cast<uint32_t>(PQgetvalue(r, i, 0));

        auto block = chain_.fetch_block_by_number(block_num);
        if(!block) {
            continue;
        }

        for(auto& tx : block->transactions) {
            if(tx.trx.id() == trx_id) {
                auto var = fc::variant();
                chain_.get_abi_serializer().to_variant(tx.trx, var);

                auto mv = fc::mutable_variant_object(var);
                mv["block_num"] = block_num;
                mv["block_id"]  = block->id();

                return response_ok(id, mv);
            }
        }
    }    
    EVT_THROW(chain::unknown_transaction_exception, "Cannot find transaction: ${t}", ("t", trx_id));
}

PREPARE_SQL_ONCE(gtrxs_plan0, "SELECT block_num, trx_id FROM transactions WHERE keys && $1 ORDER BY timestamp DESC LIMIT $2 OFFSET $3;")
PREPARE_SQL_ONCE(gtrxs_plan1, "SELECT block_num, trx_id FROM transactions WHERE keys && $1 ORDER BY timestamp ASC  LIMIT $2 OFFSET $3;")

int
pg_query::get_transactions_async(int id, const read_only::get_transactions_params& params) {
    using namespace __internal;

    int s = 0, t = 10;
    if(params.skip.valid()) {
        s = *params.skip;
    }
    if(params.take.valid()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto keys_buf = fmt::memory_buffer();
    format_array_to(keys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = std::string();
    if(params.dire.valid() && *params.dire == direction::asc) {
        stmt = fmt::format(fmt("EXECUTE gtrxs_plan0('{}',{},{});"), fmt::to_string(keys_buf), t, s);
    }
    else {
        stmt = fmt::format(fmt("EXECUTE gtrxs_plan1('{}',{},{});"), fmt::to_string(keys_buf), t, s);
    }

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get transactions command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetTransactions);
}

int
pg_query::get_transactions_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get transaction failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, "[]"); // return empty
    }

    auto results = fc::variants();
    for(int i = 0; i < n; i++) {
        auto trx_id    = transaction_id_type(std::string(PQgetvalue(r, i, 1), PQgetlength(r, i, 1)));
        auto block_num = boost::lexical_cast<uint32_t>(PQgetvalue(r, i, 0));

        auto block = chain_.fetch_block_by_number(block_num);
        if(!block) {
            continue;
        }

        for(auto& tx : block->transactions) {
            if(tx.trx.id() == trx_id) {
                auto var = fc::variant();
                chain_.get_abi_serializer().to_variant(tx.trx, var);

                auto mv = fc::mutable_variant_object(var);
                mv["block_num"] = block_num;
                mv["block_id"]  = block->id();

                results.emplace_back(mv);
                break;
            }
        }
    }    
    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gfi_plan, "SELECT sym_id FROM fungibles ORDER BY sym_id ASC LIMIT $1 OFFSET $2;");

int
pg_query::get_fungible_ids_async(int id, const read_only::get_fungible_ids_params& params) {
    using namespace __internal;

    int s = 0, t = 100;
    if(params.skip.valid()) {
        s = *params.skip;
    }
    if(params.take.valid()) {
        t = *params.take;
        EVT_ASSERT(t <= 100, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 100 per query");
    }

    auto stmt = fmt::format(fmt("EXECUTE gfi_plan({},{});"), t, s);

    auto r = PQsendQuery(conn_, stmt.c_str());
    EVT_ASSERT(r == 1, chain::postgres_send_exception, "Send get transactions command failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

    return queue(id, kGetFungibleIds);
}

int
pg_query::get_fungible_ids_resume(int id, pg_result const* r) {
    using namespace __internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungible ids failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, "[]"); // return empty
    }

    auto buf = fmt::memory_buffer();
    fmt::format_to(buf, "[");
    for(int i = 0; i < n; i++) {
        fmt::format_to(buf, "{}", PQgetvalue(r, i, 0));
        if(i < n - 1) {
            fmt::format_to(buf, ",");
        }
    }
    fmt::format_to(buf, "]");

    return response_ok(id, fmt::to_string(buf));
}

}  // namepsace evt
