/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <deque>
#include <boost/noncopyable.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/contracts/types.hpp>
#include <functional>
#include <rocksdb/options.h>

namespace rocksdb {
class DB;
}  // namespace rocksdb

namespace evt { namespace chain {

using namespace evt::chain::contracts;
using read_fungible_func = std::function<bool(const asset&)>;

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
        void accept() { _accept = 1; }
        void squash() { _accept = 1; _token_db.pop_back_savepoint(); }
        void undo()   { _accept = 1; _token_db.rollback_to_latest_savepoint(); }

        int seq() const { return _seq; }

    private:
        token_database& _token_db;
        int             _seq;
        int             _accept;
    };

public:
    token_database()
        : db_(nullptr)
        , read_opts_()
        , write_opts_()
        , tokens_handle_(nullptr)
        , assets_handle_(nullptr) {}
    token_database(const fc::path& dbpath);
    ~token_database();

public:
    int initialize(const fc::path& dbpath);

public:
    int add_domain(const domain_def&);
    int exists_domain(const domain_name&) const;
    int issue_tokens(const issuetoken&);
    int exists_token(const domain_name&, const token_name&) const;
    int add_group(const group_def&);
    int exists_group(const group_name&) const;
    int add_delay(const delay_def&);
    int exists_delay(const proposal_name&) const;
    int add_fungible(const fungible_def&);
    int exists_fungible(const symbol) const;
    int exists_fungible(const fungible_name& sym_name) const;

    int update_asset(const public_key_type& address, const asset&);
    int exists_any_asset(const public_key_type& address) const;
    int exists_asset(const public_key_type& address, const symbol) const;

    int read_domain(const domain_name&, domain_def&) const;
    int read_token(const domain_name&, const token_name&, token_def&) const;
    int read_group(const group_name&, group_def&) const;
    int read_delay(const proposal_name&, delay_def&) const;

    int read_fungible(const symbol, fungible_def&) const;
    int read_fungible(const fungible_name& sym_name, fungible_def&) const;
    int read_asset(const public_key_type& address, const symbol, asset&) const;
    // this function returns asset(0, symbol) when there's no asset key in address
    // instead of throwing an exception
    int read_asset_no_throw(const public_key_type& address, const symbol, asset&) const;
    int read_all_assets(const public_key_type& address, const read_fungible_func&) const;

    int update_domain(const domain_def&);
    int update_group(const group_def&);
    int update_token(const token_def&);
    int update_delay(const delay_def&);
    int update_fungible(const fungible_def&);

public:
    int add_savepoint(int32_t seq);
    int rollback_to_latest_savepoint();
    int pop_savepoints(int32_t until);
    int pop_back_savepoint();

    session new_savepoint_session(int seq);
    session new_savepoint_session();

    size_t get_savepoints_size() const { return savepoints_.size(); }

private:
    int should_record() { return !savepoints_.empty(); }
    int record(int type, void* data);
    int free_savepoint(savepoint&);

private:
    rocksdb::DB*                 db_;
    rocksdb::ReadOptions         read_opts_;
    rocksdb::WriteOptions        write_opts_;

    rocksdb::ColumnFamilyHandle* tokens_handle_;
    rocksdb::ColumnFamilyHandle* assets_handle_;  

    std::deque<savepoint>        savepoints_;
};

}}  // namespace evt::chain
