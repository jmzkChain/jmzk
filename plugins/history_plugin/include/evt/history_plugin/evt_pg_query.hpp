/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <string>
#include <boost/noncopyable.hpp>
#include <evt/chain/block_state.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/contracts/types.hpp>

struct pg_conn;

namespace evt {
namespace chain { namespace contracts {
struct abi_serializer;
}}  // namespace chain::

using namespace evt::chain::contracts;

#define PG_OK   1
#define PG_FAIL 0


class pg_query : boost::noncopyable {
public:
    pg_query() : conn_(nullptr) {}

public:
    int connect(const std::string& conn);
    int close();
    int prepare_stmts();

public:
    fc::variant get_tokens(const std::vector<chain::public_key_type>& pkeys, const fc::optional<domain_name>& domain);

private:
    pg_conn* conn_;
};