/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>
#include <functional>
#include <mongocxx/database.hpp>
#include <evt/chain/types.hpp>
#include <fc/container/flat_fwd.hpp>

namespace evt {

using evt::chain::public_key_type;

class wallet_query {
public:
    wallet_query(const mongocxx::database& db) : db_(db) {}

public:
    fc::flat_set<std::string> get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_domains_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_groups_by_public_keys(const std::vector<public_key_type>& pkeys);

private:
    mongocxx::database db_;
};

}  // namespace evt
