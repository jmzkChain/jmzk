/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/

#pragma once
#include <functional>
#include <eosio/chain/contracts/types.hpp>

namespace rocksdb {
class DB;
} // namespace rocksdb

namespace evt { namespace chain {

using namespace eosio::chain::contracts;

using update_token_type_func = std::function<void(token_type_def&)>;
using update_token_func = std::function<void(token_def&)>;
using update_group_func = std::function<void(group_def&)>;
using read_token_type_func = std::function<void(const token_type_def&)>;
using read_token_func = std::function<void(const token_def&)>;
using read_group_func = std::function<void(const group_def&)>;

enum tokendb_error {
    token_type_existed = -1,
    not_found_token_type = -2,
    group_existed = -3,
    not_found_group = -4,
    token_id_existed = -5,
    not_found_token_id = -6,
    rocksdb_err = -7
};

class token_db {
public:
    token_db() : db_(nullptr) {}

public:
    int initialize(const std::string& dbpath);

public:
    int add_new_token_type(const token_type_def&);
    int exists_token_type(const token_type_name);
    int issue_tokens(const issuetoken&);
    int exists_token(const token_type_name, const token_id id);
    int add_group(const group_def&);
    int exists_group(const group_key&);

    int update_token_type(const token_type_name, const update_token_type_func&);
    int read_token_type(const token_type_name, const read_token_type_func&);
    int update_token(const token_type_name, const token_id, const update_token_func&);
    int read_token(const token_type_name, const token_id, const read_token_func&);
    int update_group(const group_key&, const update_group_func&);
    int read_group(const group_key&, const read_group_func&);

private:
    rocksdb::DB*    db_;
};

}}  // namespace evt::chain

