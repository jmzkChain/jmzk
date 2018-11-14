/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/postgres_plugin/evt_pg.hpp>

#define FMT_STRING_ALIAS 1
#include <fmt/format.h>
#include <libpq-fe.h>
#include <boost/lexical_cast.hpp>
#include <fc/io/json.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/postgres_plugin/copy_context.hpp>

namespace evt {

namespace __internal {

auto create_blocks_table = "CREATE TABLE IF NOT EXISTS public.blocks                            \
                            (                                                                   \
                                block_id        character(64)            NOT NULL,              \
                                block_num       integer                  NOT NULL,              \
                                prev_block_id   character(64)            NOT NULL,              \
                                \"timestamp\"   timestamp with time zone NOT NULL,              \
                                trx_merkle_root character(64)            NOT NULL,              \
                                trx_count       integer                  NOT NULL,              \
                                producer        character varying(21)    NOT NULL,              \
                                pending         boolean                  NOT NULL DEFAULT true, \
                                created_at      timestamp with time zone NOT NULL DEFAULT now() \
                            )                                                                   \
                            WITH (                                                              \
                                OIDS = FALSE                                                    \
                            )                                                                   \
                            TABLESPACE pg_default;                                              \
                                                                                                \
                            CREATE INDEX IF NOT EXISTS block_id_index                           \
                                ON public.blocks USING btree                                    \
                                (block_id)                                                      \
                                TABLESPACE pg_default;                                          \
                                                                                                \
                            CREATE INDEX IF NOT EXISTS block_num_index                          \
                                ON public.blocks USING btree                                    \
                                (block_num)                                                     \
                                TABLESPACE pg_default;";

auto create_trxs_table = "CREATE TABLE IF NOT EXISTS public.transactions                    \
                          (                                                                 \
                              trx_id        character(64)            NOT NULL,              \
                              seq_num       integer                  NOT NULL,              \
                              block_id      character(64)            NOT NULL,              \
                              block_num     integer                  NOT NULL,              \
                              action_count  integer                  NOT NULL,              \
                              \"timestamp\" timestamp with time zone NOT NULL,              \
                              expiration    timestamp with time zone NOT NULL,              \
                              max_charge    integer                  NOT NULL,              \
                              payer         character(53)            NOT NULL,              \
                              pending       boolean                  NOT NULL DEFAULT true, \
                              type          character varying(7)     NOT NULL,              \
                              status        character varying(9)     NOT NULL,              \
                              signatures    character(120)[]         NOT NULL,              \
                              keys          character(53)[]          NOT NULL,              \
                              elapsed       integer                  NOT NULL,              \
                              charge        integer                  NOT NULL,              \
                              suspend_name  character varying(21),                          \
                              created_at    timestamp with time zone NOT NULL DEFAULT now() \
                          )                                                                 \
                          WITH (                                                            \
                              OIDS = FALSE                                                  \
                          )                                                                 \
                          TABLESPACE pg_default;                                            \
                                                                                            \
                          CREATE INDEX IF NOT EXISTS block_num_index                        \
                              ON public.transactions USING btree                            \
                              (block_num)                                                   \
                              TABLESPACE pg_default;";

auto create_actions_table = "CREATE TABLE IF NOT EXISTS public.actions                                    \
                             (                                                              \
                                 block_id   character(64)            NOT NULL,              \
                                 block_num  integer                  NOT NULL,              \
                                 trx_id     character varying(64)    NOT NULL,              \
                                 seq_num    integer                  NOT NULL,              \
                                 name       character varying(13)    NOT NULL,              \
                                 domain     character varying(21)    NOT NULL,              \
                                 key        character varying(21)    NOT NULL,              \
                                 data       jsonb                    NOT NULL,              \
                                 created_at timestamp with time zone NOT NULL DEFAULT now() \
                             )                                                              \
                             WITH (                                                         \
                                 OIDS = FALSE                                               \
                             )                                                              \
                             TABLESPACE pg_default;                                         \
                                                                                            \
                             CREATE INDEX IF NOT EXISTS trx_id_index                        \
                                 ON public.actions USING btree                              \
                                 (trx_id)                                                   \
                                 TABLESPACE pg_default;";

}  // namespace __internal

int
pg::connect(const std::string& conn) {
    conn_ = PQconnectdb(conn.c_str());

    auto status = PQstatus(conn_);
    EVT_ASSERT(status == CONNECTION_OK, chain::postgresql_connection_exception, "Connect failed");

    return PG_OK;
}

int
pg::close() {
    FC_ASSERT(conn_);
    PQfinish(conn_);
    conn_ = nullptr;

    return PG_OK;
}

int
pg::create_db(const std::string& db) {
    auto sql = "CREATE DATABASE {}      \
                    WITH                \
                    ENCODING = 'UTF8'   \
                    LC_COLLATE = 'C'    \
                    LC_CTYPE = 'C'      \
                    CONNECTION LIMIT = -1;";
    auto stmt = fmt::format(sql, db);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Create database failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    PQclear(r);
    return PG_OK;
}

int
pg::drop_db(const std::string& db) {
    auto sql = "DROP DATABASE {};";
    auto stmt = fmt::format(sql, db);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Drop database failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    PQclear(r);
    return PG_OK;
}

int
pg::exists_db(const std::string& db) {
    auto sql = "SELECT EXISTS(                                          \
                    SELECT datname                                      \
                    FROM pg_catalog.pg_database WHERE datname = '{}'    \
                );";
    auto stmt = fmt::format(sql, db);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgresql_exec_exception, "Check if database existed failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto v = PQgetvalue(r, 0, 0);
    if(strcmp(v, "t") == 0) {
        PQclear(r);
        return PG_OK;
    }
    else {
        PQclear(r);
        return PG_FAIL;
    }
}

int
pg::is_table_empty(const std::string& table) {
    auto stmt = fmt::format("SELECT block_id FROM {} LIMIT 1;", table);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgresql_exec_exception, "Get one block id failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    if(PQntuples(r) == 0) {
        PQclear(r);
        return PG_OK;
    }

    PQclear(r);
    return PG_FAIL;
}

int
pg::prepare_tables() {
    using namespace __internal;

    const char* stmts[] = { create_blocks_table, create_trxs_table, create_actions_table };
    for(auto stmt : stmts) {
        auto r = PQexec(conn_, stmt);
        EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Create table failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

        PQclear(r);
    }
    return PG_OK;
}

int
pg::drop_table(const std::string& table) {
    auto sql = "DROP TABLE IF EXISTS {};";
    auto stmt = fmt::format(sql, table);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Drop table failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    PQclear(r);
    return PG_OK;
}

int
pg::prepare_stmts() {
    return PG_OK;
}

copy_context
pg::new_copy_context() {
    return copy_context(*this);
}

int
pg::block_copy_to(const std::string& table, const std::string& data) {
    auto stmt = fmt::format("COPY {} FROM STDIN;", table);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_COPY_IN, chain::postgresql_exec_exception, "Not expected COPY response, detail: ${s}", ("s",PQerrorMessage(conn_)));
    PQclear(r);

    auto nr = PQputCopyData(conn_, data.data(), (int)data.size());
    EVT_ASSERT(nr == 1, chain::postgresql_exec_exception, "Put data into COPY stream failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto nr2 = PQputCopyEnd(conn_, NULL);
    EVT_ASSERT(nr == 1, chain::postgresql_exec_exception, "Close data into COPY stream failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto r2 = PQgetResult(conn_);
    EVT_ASSERT(PQresultStatus(r2) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Execute COPY command failed, detail: ${s}", ("s",PQerrorMessage(conn_)));
    PQclear(r2);

    return PG_OK;
}

void
pg::commit_copy_context(copy_context& wctx) {
    if(wctx.blocks_copy_.size() > 0) {
        block_copy_to("blocks", fmt::to_string(wctx.blocks_copy_));
    }
    if(wctx.trxs_copy_.size() > 0) {
        block_copy_to("transactions", fmt::to_string(wctx.trxs_copy_));
    }
    if(wctx.actions_copy_.size() > 0) {
        block_copy_to("actions", fmt::to_string(wctx.actions_copy_));
    }    
}

int
pg::add_block(copy_context& wctx, const block_ptr block) {
    fmt::format_to(wctx.blocks_copy_,
        fmt("{}\t{:d}\t{}\t{}\t{}\t{:d}\t{}\tf\tnow\n"),
        block->id.str(),
        (int32_t)block->block_num,
        block->header.previous.str(),
        (std::string)block->header.timestamp.to_time_point(),
        block->header.transaction_mroot.str(),
        block->block->transactions.size(),
        (std::string)block->header.producer
        );
    return PG_OK;
}

int
pg::add_trx(copy_context& wctx,
            const trx_recept_t& trx,
            const trx_t& strx,
            const std::string& block_id,
            int block_num,
            const std::string& ts,
            const chain::chain_id_type& chain_id,
            int seq_num,
            int elapsed,
            int charge) {
    fmt::format_to(wctx.trxs_copy_,
        fmt("{}\t{:d}\t{}\t{}\t{:d}\t{}\t{}\t{:d}\t{}\tf\t{}\t{}\t"),
        strx.id().str(),
        seq_num,
        block_id,
        block_num,
        (int32_t)strx.actions.size(),
        ts,
        (std::string)strx.expiration,
        (int32_t)strx.max_charge,
        (std::string)strx.payer,
        (std::string)trx.type,
        (std::string)trx.status
        );;

    // signatures
    fmt::format_to(wctx.trxs_copy_, fmt("{{"));
    for(auto i = 0u; i < strx.signatures.size() - 1; i++) {
        auto& sig = strx.signatures[i];
        fmt::format_to(wctx.trxs_copy_, fmt("\"{}\","), (std::string)sig);
    }
    fmt::format_to(wctx.trxs_copy_, fmt("\"{}\"}}\t"), (std::string)strx.signatures[strx.signatures.size()-1]);

    // keys
    auto keys = strx.get_signature_keys(chain_id);
    fmt::format_to(wctx.trxs_copy_, fmt("{{"));
    for(auto i = 0u; i < keys.size(); i++) {
        auto& key = *keys.nth(i);
        fmt::format_to(wctx.trxs_copy_, fmt("\"{}\","), (std::string)key);
    }
    fmt::format_to(wctx.trxs_copy_, fmt("\"{}\"}}\t"), (std::string)*keys.nth(keys.size()-1));

    // traces
    fmt::format_to(wctx.trxs_copy_, fmt("{}\t{}\t"), elapsed, charge);

    // extenscions
    auto has_ext = 0;
    for(auto& ext : strx.transaction_extensions) {
        if(std::get<0>(ext) == (uint16_t)chain::transaction_ext::suspend_name) {
            auto& v    = std::get<1>(ext);
            auto  name = std::string(v.cbegin(), v.cend());

            fmt::format_to(wctx.trxs_copy_, fmt("{}\t"), name);
            has_ext = 1;
            break;
        }
    }

    if(has_ext) {
        fmt::format_to(wctx.trxs_copy_, fmt("now\n"));
    }
    else {
        fmt::format_to(wctx.trxs_copy_, fmt("\\N\tnow\n"));
    }

    return PG_OK;
}

int
pg::add_action(copy_context& wctx, const action_t& act, const std::string& block_id, int block_num, const std::string& trx_id, int seq_num, const chain::contracts::abi_serializer& abi) {
    auto data = abi.binary_to_variant(abi.get_action_type(act.name), act.data);

    fmt::format_to(wctx.actions_copy_,
        fmt("{}\t{:d}\t{}\t{:d}\t{}\t{}\t{}\t{}\tnow\n"),
        block_id,
        block_num,
        trx_id,
        seq_num,
        act.name.to_string(),
        act.domain.to_string(),
        act.key.to_string(),
        fc::json::to_string(data)
        );

    return PG_OK;
}

int
pg::get_latest_block_id(std::string& block_id) {
    auto stmt = "SELECT block_id FROM blocks ORDER BY block_num DESC LIMIT 1;";

    auto r = PQexec(conn_, stmt);
    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, chain::postgresql_exec_exception, "Get latest block id failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    if(PQntuples(r) == 0) {
        PQclear(r);
        return PG_FAIL;
    }

    auto v = PQgetvalue(r, 0, 0);
    block_id = v;

    PQclear(r);
    return PG_OK;
}

int
pg::set_block_irreversible(const std::string& block_id) {
    static int prepared = 0;
    if(!prepared) {
        auto r = PQprepare(conn_, "sbi_plan", "UPDATE blocks SET pending = false WHERE block_id = $1", 0, NULL);
        EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Prepare set block irreversible failed, detail: ${s}", ("s",PQerrorMessage(conn_)));
        PQclear(r);

        prepared = 1;
    }

    const char* values[] = { block_id.c_str() }; 
    auto r = PQexecPrepared(conn_, "sbi_plan", 1, values, NULL, NULL, 0);
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, chain::postgresql_exec_exception, "Set block irreversiblefailed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto rows = PQcmdTuples(r);
    if(boost::lexical_cast<int>(rows) > 0) {
        PQclear(r);
        return PG_OK;
    }

    PQclear(r);
    return PG_FAIL;
}

}  // namepsace evt