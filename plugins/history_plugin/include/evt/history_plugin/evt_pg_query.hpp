/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <queue>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <evt/chain/block_state.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/history_plugin/history_plugin.hpp>

struct pg_conn;
struct pg_result;

namespace evt {

namespace chain { namespace contracts {
struct abi_serializer;
}}  // namespace chain::

using namespace evt::chain::contracts;
using namespace evt::history_apis;

#define PG_OK   1
#define PG_FAIL 0


class pg_query : boost::noncopyable {
private:
    struct task {
    public:
        task(int id, int type) : id(id), type(type) {}

    public:
        int id;
        int type;
    };

public:
    pg_query(boost::asio::io_context& io_serv)
        : conn_(nullptr), io_serv_(io_serv), socket_(io_serv) {}

public:
    int connect(const std::string& conn);
    int close();
    int prepare_stmts();
    int begin_poll_read();

public:
    int get_tokens_async(int id, const read_only::get_tokens_params& params);
    int get_tokens_resume(int id, pg_result const*);

    int get_domains_async(int id, const read_only::get_params& params);
    int get_domains_resume(int id, pg_result const*);

    int get_groups_async(int id, const read_only::get_params& params);
    int get_groups_resume(int id, pg_result const*);

    int get_fungibles_async(int id, const read_only::get_params& params);
    int get_fungibles_resume(int id, pg_result const*);

    int get_actions_async(int id, const read_only::get_actions_params& params);
    int get_actions_resume(int id, pg_result const*);

private:
    int queue(int id, int task);
    int poll_read();

private:
    pg_conn* conn_;

    std::queue<task>             tasks_;
    boost::asio::io_context&     io_serv_;
    boost::asio::ip::tcp::socket socket_;
};

}  // namespace evt