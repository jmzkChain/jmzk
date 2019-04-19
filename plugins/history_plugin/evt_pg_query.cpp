/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/history_plugin/evt_pg_query.hpp>

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#include <functional>
#include <fmt/format.h>
#include <libpq-fe.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <fc/io/json.hpp>
#include <evt/chain/block_header.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/http_plugin/http_plugin.hpp>

namespace evt {

namespace internal {

struct prepare_register {
public:
    std::map<std::string, std::string> stmts;

public:
    static prepare_register& instance() {
        static prepare_register i;
        return i;
    }
};

struct insert_prepare {
    insert_prepare(const std::string& name, const std::string sql) {
        prepare_register::instance().stmts[name] = sql;
    }
};

#define PREPARE_SQL_ONCE(name, sql) \
    internal::insert_prepare ipsql_##name(#name, sql);

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
    kGetFungiblesBalance,
    kGetTransaction,
    kGetTransactions,
    kGetFungibleIds,
    kGetTransactionActions
};

const char* call_names[] = {
    "get_tokens",
    "get_domains",
    "get_groups",
    "get_fungibles",
    "get_actions",
    "get_fungible_actions",
    "get_fungibles_balance",
    "get_transaction",
    "get_transactions",
    "get_fungible_ids",
    "get_transaction_actions"
};

template<typename T>
int
response_ok(int id, const T& obj) {
    app().get_plugin<http_plugin>().set_deferred_response(id, 200, fc::json::to_string(obj));
    return PG_OK;
}

template<>
int
response_ok<std::string>(int id, const std::string& str) {
    app().get_plugin<http_plugin>().set_deferred_response(id, 200, str);
    return PG_OK;
}

// This function is used to fix the representation of timestamp returned by postgres
// Use 'T' as the separate the date and time to follow the ISO 8601 standard
char*
fix_pg_timestamp(char* str) {
    auto sz = strlen(str);
    if(sz <= 10) {
        return str;
    }
    str[10] = 'T';
    return str;
}

}  // namespace internal

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
    for(auto it : internal::prepare_register::instance().stmts) {
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
pg_query::queue(int id, int task, std::string&& stmt) {
    tasks_.emplace(id, task, std::move(stmt));
    
    if(!sending_) {
        send_once();
    }
    return PG_OK;
}

int
pg_query::send_once() {
    using namespace internal;

    assert(!tasks_.empty());
    auto& t = tasks_.front();

    auto r = PQsendQuery(conn_, t.stmt.c_str());
    if(r == 1) {
        sending_ = true;
        return PG_OK;
    }

    try {
        EVT_THROW2(chain::postgres_send_exception,
            "Send '{}' query command failed, detail: {}", call_names[t.id], PQerrorMessage(conn_));
    }
    catch(...) {
        app().get_plugin<http_plugin>().handle_async_exception(t.id, "history", call_names[t.id], "");
    }

    tasks_.pop();
    if(!tasks_.empty()) {
        // send next one
        return send_once();
    }
    return PG_FAIL;
}

int
pg_query::poll_read() {
    using namespace internal;

    bool busy = false;
    while(1) {
        auto r = PQconsumeInput(conn_);
        EVT_ASSERT(r, chain::postgres_poll_exception, "Poll messages from postgres failed, detail: ${d}", ("d",PQerrorMessage(conn_)));

        if(PQisBusy(conn_)) {
            busy = true;
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
            case kGetFungiblesBalance: {
                get_fungibles_balance_resume(t.id, re);
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
            case kGetTransactionActions: {
                get_transaction_actions_resume(t.id, re);
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
    if(!busy && !tasks_.empty()) {
        // send next one
        if(send_once() == PG_FAIL) {
            // no send
            sending_ = false;
        }
    }
    else {
        sending_ = false;
    }
    return PG_OK;
}

PREPARE_SQL_ONCE(gt_plan,  "SELECT domain, name FROM tokens WHERE $1 @> owner AND domain = $2");
PREPARE_SQL_ONCE(gt_plan2, "SELECT domain, name FROM tokens WHERE $1 @> owner");  // without domain filter

int
pg_query::get_tokens_async(int id, const read_only::get_tokens_params& params) {
    using namespace internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = std::string();
    if(params.domain.has_value()) {
        stmt = fmt::format(fmt("EXECUTE gt_plan ('{}','{}');"), fmt::to_string(pkeys_buf), (std::string)*params.domain);
    }
    else {
        stmt = fmt::format(fmt("EXECUTE gt_plan2 ('{}');"), fmt::to_string(pkeys_buf));
    }

    return queue(id, kGetTokens, std::move(stmt));
}

int
pg_query::get_tokens_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get tokens failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto results = fc::mutable_variant_object();
    for(int i = 0; i < n; i++) {
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
    using namespace internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gd_plan ('{}')"), fmt::to_string(pkeys_buf));
    return queue(id, kGetDomains, std::move(stmt));
}

int
pg_query::get_domains_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get domains failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto results = std::vector<const char*>();
    for(int i = 0; i < n; i++) {
        auto name = PQgetvalue(r, i, 0);
        results.emplace_back(name);
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gg_plan, "SELECT name FROM groups WHERE key = ANY($1);");

int
pg_query::get_groups_async(int id, const read_only::get_params& params) {
    using namespace internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gg_plan ('{}')"), fmt::to_string(pkeys_buf));
    return queue(id, kGetGroups, std::move(stmt));
}

int
pg_query::get_groups_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get groups failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto results = std::vector<const char*>();
    for(int i = 0; i < n; i++) {
        auto name = PQgetvalue(r, i, 0);
        results.emplace_back(name);
    }

    return response_ok(id, results);
}

PREPARE_SQL_ONCE(gf_plan, "SELECT sym_id FROM fungibles WHERE creator = ANY($1);");

int
pg_query::get_fungibles_async(int id, const read_only::get_params& params) {
    using namespace internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = fmt::format(fmt("EXECUTE gf_plan ('{}')"), fmt::to_string(pkeys_buf));
    return queue(id, kGetFungibles, std::move(stmt));
}

int
pg_query::get_fungibles_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungibles failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto results = std::vector<int64_t>();
    for(int i = 0; i < n; i++) {
        auto sym_id = PQgetvalue(r, i, 0);
        results.emplace_back(boost::lexical_cast<int64_t>(sym_id));
    }

    return response_ok(id, results);
}

auto ga_plan0 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                      FROM actions
                      JOIN transactions ON actions.trx_id = transactions.trx_id
                      WHERE domain = $1
                      ORDER BY actions.global_seq {0}
                      LIMIT $2 OFFSET $3
                      )sql";

// with key filter
auto ga_plan1 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                      FROM actions
                      JOIN transactions ON actions.trx_id = transactions.trx_id
                      WHERE domain = $1 AND key = $2
                      ORDER BY actions.global_seq {0}
                      LIMIT $3 OFFSET $4
                      )sql";

// with name filter
auto ga_plan2 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                      FROM actions
                      JOIN transactions ON actions.trx_id = transactions.trx_id
                      WHERE domain = $1 AND name = ANY($2)
                      ORDER BY actions.global_seq {0}
                      LIMIT $3 OFFSET $4
                      )sql";

// with key and name filter
auto ga_plan3 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                      FROM actions
                      JOIN transactions ON actions.trx_id = transactions.trx_id
                      WHERE domain = $1 AND key = $2 AND name = ANY($3)
                      ORDER BY actions.global_seq {0}
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
    using namespace internal;

    int s = 0, t = 10;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto stmt = std::string();

    int j = 0;
    if(params.dire.has_value() && *params.dire == direction::asc) {
        j += 1;
    }
    if(params.key.has_value()) {
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

    return queue(id, kGetActions, std::move(stmt));
}

int
pg_query::get_actions_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get actions failed, detail: ${s}", ("s",PQerrorMessage(conn_)));
    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto builder = fmt::memory_buffer();

    fmt::format_to(builder, "[");
    for(int i = 0; i < n; i++) {
        fmt::format_to(builder,
            fmt(R"({{"trx_id":"{}","name":"{}","domain":"{}","key":"{}","data":{},"timestamp":"{}"}})"),
            PQgetvalue(r, i, 0),
            PQgetvalue(r, i, 1),
            PQgetvalue(r, i, 2),
            PQgetvalue(r, i, 3),
            PQgetvalue(r, i, 4),
            fix_pg_timestamp(PQgetvalue(r, i, 5))
            );
        if(i < n - 1) {
            fmt::format_to(builder, ",");
        }
    }
    fmt::format_to(builder, "]");

    return response_ok(id, fmt::to_string(builder));
}

auto gfa_plan0 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                       FROM actions
                       JOIN transactions ON actions.trx_id = transactions.trx_id
                       WHERE
                           domain = '.fungible'
                           AND key = $1
                           AND name = ANY('{{"issuefungible","transferft","recycleft","evt2pevt","everipay","paybonus"}}')
                       ORDER BY actions.created_at {0}, actions.seq_num {0}
                       LIMIT $2 OFFSET $3
                       )sql";

// with address filter
auto gfa_plan1 = R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                       FROM actions
                       JOIN transactions ON actions.trx_id = transactions.trx_id
                       WHERE
                           domain = '.fungible'
                           AND key = $1
                           AND name = ANY('{{"issuefungible","transferft","recycleft","evt2pevt","everipay","paybonus"}}')
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
    using namespace internal;

    int s = 0, t = 10;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto stmt = std::string();

    int j = 0;
    if(params.dire.has_value() && *params.dire == direction::asc) {
        j += 1;
    }
    if(params.addr.has_value()) {
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

    return queue(id, kGetFungibleActions, std::move(stmt));
}

int
pg_query::get_fungible_actions_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungible actions failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto builder = fmt::memory_buffer();

    fmt::format_to(builder, "[");
    for(int i = 0; i < n; i++) {
        fmt::format_to(builder,
            fmt(R"({{"trx_id":"{}","name":"{}","domain":"{}","key":"{}","data":{},"timestamp":"{}"}})"),
            PQgetvalue(r, i, 0),
            PQgetvalue(r, i, 1),
            PQgetvalue(r, i, 2),
            PQgetvalue(r, i, 3),
            PQgetvalue(r, i, 4),
            fix_pg_timestamp(PQgetvalue(r, i, 5))
            );
        if(i < n - 1) {
            fmt::format_to(builder, ",");
        }
    }
    fmt::format_to(builder, "]");

    return response_ok(id, fmt::to_string(builder));
}

PREPARE_SQL_ONCE(gfb_plan, "SELECT address, sym_ids FROM ft_holders WHERE address = $1;");

int
pg_query::get_fungibles_balance_async(int id, const read_only::get_fungibles_balance_params& params) {
    using namespace internal;

    auto stmt = fmt::format(fmt("EXECUTE gfb_plan('{}');"), (std::string)params.addr);
    return queue(id, kGetFungiblesBalance, std::move(stmt));
}

#define READ_DB_ASSET(ADDR, SYM_ID, VALUEREF)                                                         \
    try {                                                                                             \
        auto str = std::string();                                                                     \
        tokendb.read_asset(ADDR, SYM_ID, str);                                                        \
                                                                                                      \
        extract_db_value(str, VALUEREF);                                                              \
    }                                                                                                 \
    catch(token_database_exception&) {                                                                \
        EVT_THROW2(balance_exception, "There's no balance left in {} with sym id: {}", ADDR, SYM_ID); \
    }

int
pg_query::get_fungibles_balance_resume(int id, pg_result const* r) {
    using namespace internal;
    using namespace boost::algorithm;
    using namespace chain;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get transaction failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto  addr    = PQgetvalue(r, 0, 0);
    auto  arr     = PQgetvalue(r, 0, 1);
    auto  len     = strlen(arr);
    auto  vars    = variants();
    auto& tokendb = chain_.token_db();

    auto it = split_iterator(arr + 1, arr + len - 1, first_finder(","));
    for(; !it.eof(); it++) {
        auto sym_id = boost::lexical_cast<uint32_t>(it->begin(), it->size());

        property prop;
        READ_DB_ASSET(address(addr), sym_id, prop);

        auto as  = asset(prop.amount, prop.sym);
        auto var = fc::variant();
        fc::to_variant(as, var);

        vars.emplace_back(std::move(var));
    }

    return response_ok(id, vars);
}

PREPARE_SQL_ONCE(gtrx_plan, "SELECT block_num, trx_id FROM transactions WHERE trx_id = $1;");

int
pg_query::get_transaction_async(int id, const read_only::get_transaction_params& params) {
    using namespace internal;

    auto stmt = fmt::format(fmt("EXECUTE gtrx_plan('{}');"), (std::string)params.id);
    return queue(id, kGetTransaction, std::move(stmt));
}

int
pg_query::get_transaction_resume(int id, pg_result const* r) {
    using namespace internal;

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

        auto& abi      = chain_.get_abi_serializer();
        auto& exec_ctx = chain_.get_execution_context();
        for(auto& tx : block->transactions) {
            if(tx.trx.id() == trx_id) {
                auto var = fc::variant();
                abi.to_variant(tx.trx, var, exec_ctx);

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
    using namespace internal;

    int s = 0, t = 10;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        EVT_ASSERT(t <= 20, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 20 per query");
    }

    auto keys_buf = fmt::memory_buffer();
    format_array_to(keys_buf, std::begin(params.keys), std::end(params.keys));

    auto stmt = std::string();
    if(params.dire.has_value() && *params.dire == direction::asc) {
        stmt = fmt::format(fmt("EXECUTE gtrxs_plan0('{}',{},{});"), fmt::to_string(keys_buf), t, s);
    }
    else {
        stmt = fmt::format(fmt("EXECUTE gtrxs_plan1('{}',{},{});"), fmt::to_string(keys_buf), t, s);
    }

    return queue(id, kGetTransactions, std::move(stmt));
}

int
pg_query::get_transactions_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get transaction failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
    }

    auto results = fc::variants();
    for(int i = 0; i < n; i++) {
        auto trx_id    = transaction_id_type(std::string(PQgetvalue(r, i, 1), PQgetlength(r, i, 1)));
        auto block_num = boost::lexical_cast<uint32_t>(PQgetvalue(r, i, 0));

        auto block = chain_.fetch_block_by_number(block_num);
        if(!block) {
            continue;
        }

        auto& abi      = chain_.get_abi_serializer();
        auto& exec_ctx = chain_.get_execution_context();
        for(auto& tx : block->transactions) {
            if(tx.trx.id() == trx_id) {
                auto var = fc::variant();
                abi.to_variant(tx.trx, var, exec_ctx);

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
    using namespace internal;

    int s = 0, t = 100;
    if(params.skip.has_value()) {
        s = *params.skip;
    }
    if(params.take.has_value()) {
        t = *params.take;
        EVT_ASSERT(t <= 100, chain::exceed_query_limit_exception, "Exceed limit of max actions return allowed for each query, limit: 100 per query");
    }

    auto stmt = fmt::format(fmt("EXECUTE gfi_plan({},{});"), t, s);
    return queue(id, kGetFungibleIds, std::move(stmt));
}

int
pg_query::get_fungible_ids_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get fungible ids failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        return response_ok(id, std::string("[]")); // return empty
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

PREPARE_SQL_ONCE(gta_plan, R"sql(SELECT actions.trx_id, name, domain, key, data, transactions.timestamp
                                 FROM actions
                                 JOIN transactions ON actions.trx_id = transactions.trx_id
                                 WHERE actions.trx_id = $1
                                 ORDER BY actions.seq_num ASC
                                 )sql");

int
pg_query::get_transaction_actions_async(int id, const read_only::get_transaction_actions_params& params) {
    using namespace internal;

    auto stmt = fmt::format(fmt("EXECUTE gta_plan('{}');"), (std::string)params.id);
    return queue(id, kGetTransactionActions, std::move(stmt));
}

int
pg_query::get_transaction_actions_resume(int id, pg_result const* r) {
    using namespace internal;

    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgres_query_exception, "Get transaction actions failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto n = PQntuples(r);
    if(n == 0) {
        EVT_THROW(chain::unknown_transaction_exception, "Cannot find transaction");
    }

    auto builder = fmt::memory_buffer();

    fmt::format_to(builder, "[");
    for(int i = 0; i < n; i++) {
        fmt::format_to(builder,
            fmt(R"({{"trx_id":"{}","name":"{}","domain":"{}","key":"{}","data":{},"timestamp":"{}"}})"),
            PQgetvalue(r, i, 0),
            PQgetvalue(r, i, 1),
            PQgetvalue(r, i, 2),
            PQgetvalue(r, i, 3),
            PQgetvalue(r, i, 4),
            fix_pg_timestamp(PQgetvalue(r, i, 5))
            );
        if(i < n - 1) {
            fmt::format_to(builder, ",");
        }
    }
    fmt::format_to(builder, "]");

    return response_ok(id, fmt::to_string(builder));
}

}  // namepsace evt
