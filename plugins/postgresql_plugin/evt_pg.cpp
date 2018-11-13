/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/postgresql_plugin/evt_pg.hpp>
#include <fmt/format.h>

namespace evt {

namespace __internal {

auto create_blocks_table = "CREATE TABLE IF NOT EXISTS public.blocks                \
                            (                                                       \
                                block_id        character(64)            NOT NULL,  \
                                block_num       integer                  NOT NULL,  \
                                prev_block_id   character(64)            NOT NULL,  \
                                \"timestamp\"   timestamp with time zone NOT NULL,  \
                                trx_merkle_root character(64)            NOT NULL,  \
                                trx_count       integer                  NOT NULL,  \
                                producer        character varying(21)    NOT NULL,  \
                                pending         boolean                  NOT NULL,  \
                                created_at      timestamp with time zone NOT NULL   \
                            ) PARTITION BY RANGE (((block_num / 1000000)))          \
                            WITH (                                                  \
                                OIDS = FALSE                                        \
                            )                                                       \
                            TABLESPACE pg_default;                                  \
                                                                                    \
                            CREATE INDEX IF NOT EXISTS block_id_index               \
                                ON public.blocks USING btree                        \
                                (block_id)                                          \
                                TABLESPACE pg_default;";

auto create_trxs_table = "CREATE TABLE IF NOT EXISTS public.transactions        \
                          (                                                     \
                              trx_id       character(64)            NOT NULL,   \
                              seq_num      integer                  NOT NULL,   \
                              block_id     character(64)            NOT NULL,   \
                              block_num    integer                  NOT NULL,   \
                              action_count integer                  NOT NULL,   \
                              expiration   timestamp with time zone NOT NULL,   \
                              max_charge   integer                  NOT NULL,   \
                              payer        character(53)            NOT NULL,   \
                              pending      boolean                  NOT NULL,   \
                              created_at   timestamp with time zone NOT NULL,   \
                              type         character varying(7)     NOT NULL,   \
                              status       character varying(9)     NOT NULL,   \
                              signatures   character(120)[]         NOT NULL,   \
                              keys         character(53)            NOT NULL,   \
                              elapsed      bigint,                              \
                              charge       bigint,                              \
                              suspend_name character varying(21)                \
                          ) PARTITION BY RANGE (((block_num / 1000000)))        \
                          WITH (                                                \
                              OIDS = FALSE                                      \
                          )                                                     \
                          TABLESPACE pg_default;                                \
                                                                                \
                          CREATE INDEX IF NOT EXISTS block_num_index            \
                              ON public.transactions USING btree                \
                              (block_num)                                       \
                              TABLESPACE pg_default;";

auto create_actions_table = "CREATE TABLE public.actions                        \
                             (                                                  \
                                 trx_id     character varying(64)    NOT NULL,  \
                                 seq_num    integer                  NOT NULL,  \
                                 block_num  integer                  NOT NULL,  \
                                 name       character varying(13)    NOT NULL,  \
                                 domain     character varying(21)    NOT NULL,  \
                                 key        character varying(21)    NOT NULL,  \
                                 created_at timestamp with time zone NOT NULL,  \
                                 data       jsonb                    NOT NULL   \
                             ) PARTITION BY RANGE (((block_num / 1000000)))     \
                             WITH (                                             \
                                 OIDS = FALSE                                   \
                             )                                                  \
                             TABLESPACE pg_default;                             \
                                                                                \
                             CREATE INDEX IF NOT EXISTS trx_id_index            \
                                 ON public.actions USING btree                  \
                                 (trx_id)                                       \
                                 TABLESPACE pg_default;";

}  // namespace __internal

int
pg::connect(const std::string& conn) {
    conn_ = PQconnectdb(conn.c_str());
    EVT_ASSERT(PQstatus(conn_) == CONNECTION_OK, postgresql_connection_exception, "Connect failed");
}

int
pg::close() {
    FC_ASSERT(conn_);
    PQfinish(conn_);
    conn_ = nullptr;
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
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, postgresql_exec_exception, "Create database failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    PQclear(r);
    return 0;
}

int
pg::drop_db(const std::string& db) {
    auto sql = "DROP DATABASE {};";
    auto stmt = fmt::format(sql, db);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, postgresql_exec_exception, "Drop database failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    PQclear(r);
    return 0;
}

int
pg::exists_db(const std::string& db) {
    auto sql = "SELECT EXISTS(                                          \
                    SELECT datname                                      \
                    FROM pg_catalog.pg_database WHERE datname = '{}'    \
                );";
    auto stmt = fmt::format(sql, db);

    auto r = PQexec(conn_, stmt.c_str());
    EVT_ASSERT(PQresultStatus(r) == PGRES_TUPLES_OK, postgresql_exec_exception, "Check if database existed failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

    auto v = PQgetvalue(r, 0, 0);
    if(strcmp(v, "true") == 0) {
        PQclear(r);
        return true;
    }
    else {
        PQclear(r);
        return false;
    }
}

int
pg::prepare_tables() {
    using namespace __internal;

    const char* stmts[] = { create_blocks_table, create_trxs_table, create_actions_table };
    for(auto stmt : stmts) {
        auto r = PQexec(conn_, stmt.c_str());
        EVT_ASSERT(PQresultStatus(r) == PGRES_COMMAND_OK, postgresql_exec_exception, "Create table failed, detail: ${s}", ("s",PQerrorMessage(conn_)));

        PQclear(r);
    }
    return 0;
}

}  // namepsace evt