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

using update_domain_func = std::function<void(domain_def&)>;
using update_token_func = std::function<void(token_def&)>;
using update_group_func = std::function<void(group_def&)>;
using read_domain_func = std::function<void(const domain_def&)>;
using read_token_func = std::function<void(const token_def&)>;
using read_group_func = std::function<void(const group_def&)>;

enum tokendb_error {
    domain_existed = -1,
    not_found_domain = -2,
    group_existed = -3,
    not_found_group = -4,
    token_id_existed = -5,
    not_found_token_id = -6,
    rocksdb_err = -7
};

class tokendb {
public:
    tokendb() : db_(nullptr) {}

public:
    int initialize(const std::string& dbpath);

public:
    int add_domain(const domain_def&);
    int exists_domain(const domain_name);
    int issue_tokens(const issuetoken&);
    int exists_token(const domain_name, const token_name name);
    int add_group(const group_def&);
    int exists_group(const group_id&);

    int update_domain(const domain_name, const update_domain_func&);
    int read_domain(const domain_name, const read_domain_func&) const;
    int update_token(const domain_name, const token_name, const update_token_func&);
    int read_token(const domain_name, const token_name, const read_token_func&) const;
    int update_group(const group_id&, const update_group_func&);
    int read_group(const group_id&, const read_group_func&) const;

private:
    rocksdb::DB*    db_;
};

}}  // namespace evt::chain

