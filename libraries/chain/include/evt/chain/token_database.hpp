/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <deque>
#include <boost/noncopyable.hpp>
#include <evt/chain/contracts/types.hpp>
#include <functional>
#include <rocksdb/options.h>

namespace rocksdb {
class DB;
}  // namespace rocksdb

namespace evt { namespace chain {

using namespace evt::chain::contracts;

using read_domain_func  = std::function<void(const domain_def&)>;
using read_token_func   = std::function<void(const token_def&)>;
using read_group_func   = std::function<void(const group_def&)>;
using read_account_func = std::function<void(const account_def&)>;

class token_database : boost::noncopyable {
private:
    struct dbaction {
        int   type;
        void* data;
    };
    struct savepoint {
        int32_t               seq;
        const void*           rb_snapshot;
        std::vector<dbaction> actions;
    };

public:
    class session {
    public:
        session(token_database& token_db, int seq)
            : _token_db(token_db)
            , _seq(seq)
            , _accept(0) {}
        session(const session& s) = delete;
        session(session&& s)
            : _token_db(s._token_db)
            , _seq(s._seq)
            , _accept(s._accept) {
            if(!_accept) {
                s._accept = 1;
            }
        }

        ~session() {
            if(!_accept) {
                _token_db.rollback_to_latest_savepoint();
            }
        }

    public:
        void
        accept() { _accept = 1; }

    private:
        token_database& _token_db;
        int             _seq;
        int             _accept;
    };

public:
    token_database()
        : db_(nullptr)
        , read_opts_()
        , write_opts_() {}
    token_database(const fc::path& dbpath);
    ~token_database();

public:
    int initialize(const fc::path& dbpath);

public:
    int add_domain(const domain_def&);
    int exists_domain(const domain_name&) const;
    int issue_tokens(const issuetoken&);
    int exists_token(const domain_name&, const token_name& name) const;
    int add_group(const group_def&);
    int exists_group(const group_name&) const;
    int add_account(const account_def&);
    int exists_account(const account_name&) const;

    int read_domain(const domain_name&, const read_domain_func&) const;
    int read_token(const domain_name&, const token_name&, const read_token_func&) const;
    int read_group(const group_name&, const read_group_func&) const;
    int read_account(const account_name&, const read_account_func&) const;

    // specific function, use merge operator to speed up rocksdb action.
    int update_domain(const updatedomain& ud);
    int update_group(const updategroup& ug);
    int transfer_token(const transfer& tt);
    int update_account(const updateaccount& ua);

public:
    int add_savepoint(int32_t seq);
    int rollback_to_latest_savepoint();
    int pop_savepoints(int32_t until);

    session new_savepoint_session(int seq);

private:
    int
    should_record() { return !savepoints_.empty(); }
    int record(int type, void* data);
    int free_savepoint(savepoint&);

private:
    rocksdb::DB*          db_;
    rocksdb::ReadOptions  read_opts_;
    rocksdb::WriteOptions write_opts_;
    std::deque<savepoint> savepoints_;
};

}}  // namespace evt::chain
