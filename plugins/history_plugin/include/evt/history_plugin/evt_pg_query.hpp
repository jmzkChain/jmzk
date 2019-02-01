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

namespace chain {
class controller;
namespace contracts {
struct abi_serializer;
}  // namespace contracts
}  // namespace chain

using namespace evt::chain::contracts;
using namespace evt::history_apis;

#define PG_OK   1
#define PG_FAIL 0


class pg_query : boost::noncopyable {
private:
    struct task {
    public:
        task(int id, int type, std::string&& stmt)
            : id(id), type(type), stmt(std::move(stmt)) {}

    public:
        int         id;
        int         type;
        std::string stmt;
    };

public:
    pg_query(boost::asio::io_context& io_serv, controller& chain)
        : conn_(nullptr), io_serv_(io_serv), chain_(chain), socket_(io_serv) {}

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

    int get_fungible_actions_async(int id, const read_only::get_fungible_actions_params& params);
    int get_fungible_actions_resume(int id, pg_result const*);

    int get_fungibles_balance_async(int id, const read_only::get_fungibles_balance_params& params);
    int get_fungibles_balance_resume(int id, pg_result const*);

    int get_transaction_async(int id, const read_only::get_transaction_params& params);
    int get_transaction_resume(int id, pg_result const*);

    int get_transactions_async(int id, const read_only::get_transactions_params& params);
    int get_transactions_resume(int id, pg_result const*);

    int get_fungible_ids_async(int id, const read_only::get_fungible_ids_params& params);
    int get_fungible_ids_resume(int id, pg_result const*);

    int get_transaction_actions_async(int id, const read_only::get_transaction_actions_params& params);
    int get_transaction_actions_resume(int id, pg_result const*);

private:
    int queue(int id, int task, std::string&& stmt);
    int poll_read();
    int send_once();

private:
    pg_conn* conn_;

    std::queue<task>             tasks_;
    boost::asio::io_context&     io_serv_;
    chain::controller&           chain_;
    boost::asio::ip::tcp::socket socket_;
};

}  // namespace evt