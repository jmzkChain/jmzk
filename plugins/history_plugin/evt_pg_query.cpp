/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/history_plugin/evt_pg_query.hpp>

#define FMT_STRING_ALIAS 1
#include <fmt/format.h>
#include <libpq-fe.h>
#include <boost/lexical_cast.hpp>
#include <fc/io/json.hpp>
#include <evt/chain/block_header.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>

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

}  // namespace __internal


int
pg_query::connect(const std::string& conn) {
    conn_ = PQconnectdb(conn.c_str());

    auto status = PQstatus(conn_);
    EVT_ASSERT(status == CONNECTION_OK, chain::postgres_connection_exception, "Connect failed");

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

PREPARE_SQL_ONCE(gt_plan, "SELECT domain, name FROM tokens WHERE $1 <@ owner AND domain = $2");

fc::variant
pg::get_tokens(const std::vector<chain::public_key_type>& pkeys, const fc::optional<domain_name>& domain) {
    using namespace __internal;

    auto pkeys_buf = fmt::memory_buffer();
    format_array_to(pkeys_buf, std::begin(pkeys), std::end(pkeys));

    std::string d = domain.valid() ? (std::string)domain : "*";

    

    return PG_OK;
}
}  // namepsace evt