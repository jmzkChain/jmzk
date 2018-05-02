/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <deque>
#include <functional>
#include <boost/noncopyable.hpp>
#include <rocksdb/options.h>
#include <evt/chain/contracts/types.hpp>

namespace rocksdb {
class DB;
} // namespace rocksdb

namespace evt { namespace chain {

using namespace evt::chain::contracts;

using read_domain_func = std::function<void(const domain_def&)>;
using read_token_func = std::function<void(const token_def&)>;
using read_group_func = std::function<void(const group_def&)>;
using read_account_func = std::function<void(const account_def&)>;

class tokendb : boost::noncopyable {
private:
    struct dbaction {
        int     type;
        void*   data;
    };
    struct savepoint {
        int32_t                 seq;
        const void*             rb_snapshot;
        std::vector<dbaction>   actions;
    };

public:
    tokendb() : db_(nullptr), read_opts_(), write_opts_() {}
    ~tokendb();

public:
    int initialize(const fc::path& dbpath);

public:
    int add_domain(const domain_def&);
    int exists_domain(const domain_name&) const;
    int issue_tokens(const issuetoken&);
    int exists_token(const domain_name&, const token_name& name) const;
    int add_group(const group_def&);
    int exists_group(const group_id&) const;
    int add_account(const account_def&);
    int exists_account(const account_name&) const;

    int read_domain(const domain_name&, const read_domain_func&) const;
    int read_token(const domain_name&, const token_name&, const read_token_func&) const;
    int read_group(const group_id&, const read_group_func&) const;
    int read_account(const account_name&, const read_account_func&) const;

    // specific function, use merge operator to speed up rocksdb action.
    int update_domain(const updatedomain& ud);
    int update_group(const updategroup& ug);
    int transfer_token(const transfer& tt);
    int update_account(const updateowner& uo);

public:
    int add_savepoint(int32_t seq);
    int rollback_to_latest_savepoint();
    int pop_savepoints(int32_t until);

private:
    int should_record() { return !savepoints_.empty(); }
    int record(int type, void* data);
    int free_savepoint(savepoint&);

private:
    rocksdb::DB*            db_;
    rocksdb::ReadOptions    read_opts_;
    rocksdb::WriteOptions   write_opts_;
    std::deque<savepoint>   savepoints_;
};

}}  // namespace evt::chain

