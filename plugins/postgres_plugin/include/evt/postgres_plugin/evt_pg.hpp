/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <string>
#include <boost/noncopyable.hpp>

struct pg_conn;

namespace evt {

#define PG_OK   0
#define PG_FAIL 1

struct block_t {
    int a;
    int b;
};

struct trx_t {
    int a;
    int b;
};

struct action_t {
    int a;
    int b;
};

struct domain_t {
    int a;
    int b;

};
struct token_t {
    int a;
    int b;
};

struct group_t {
    int a;
    int b;
};

struct fungible_t {
    int a;
    int b;
};

class write_context;

class pg : boost::noncopyable {
public:
    pg() : conn_(nullptr) {}

public:
    int connect(const std::string& conn);
    int close();

public:
    int create_db(const std::string& db);
    int drop_db(const std::string& db);
    int exists_db(const std::string& db);
    int prepare_tables();
    int is_table_empty();

public:
    write_context& new_write_context();

public:
    int add_block(const block_t&);
    int get_block(const std::string& block_id, block_t&);
    int get_latest_block_id(std::string& block_id);
    int set_block_irreversible(const std::string& block_id);

    int add_trx(const trx_t&);
    int get_trx(const std::string& trx_id, trx_t&);

    int add_action(const action_t&);

    int add_domain(domain_t&);

    int add_token(token_t&);

    int add_group(group_t&);

    int add_fungible(fungible_t&);

private:
    pg_conn* conn_;
};



}  // namespace evt
