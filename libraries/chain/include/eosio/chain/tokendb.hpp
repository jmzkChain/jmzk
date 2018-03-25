/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/

#pragma once
#include <functional>
#include <eosio/chain/contracts/types.hpp>

namespace evt { namespace chain {

using namespace eosio::chain::contracts;

using update_token_type_func = std::function<void(token_type_def&)>;
using update_token_func = std::function<void(token_def&)>;
using read_token_type_func = std::function<void(const token_type_def&)>;
using read_token_func = std::function<void(const token_def&)>;

class token_db {
public:
    int add_new_token_type(const token_type_def&);
    int exists_token_type(const token_type_name&);
    int issue_tokens(const issuetoken&);

    int update_token_type(const token_type_name&, const update_token_type_func&);
    int read_token_type(const token_type_name&, const read_token_type_func&);
    int update_token(const token_type_name&, const token_id, const update_token_func&);
    int read_token(const token_type_name&, const token_id, const read_token_func&);

private:

};

}}  // namespace evt::chain

