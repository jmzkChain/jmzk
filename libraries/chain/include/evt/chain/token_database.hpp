/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <deque>
#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>
#include <rocksdb/options.h>
#include <evt/chain/asset.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

namespace rocksdb {
class DB;
}  // namespace rocksdb

// Use only lower 48-bit address for some pointers.
/*
    This optimization uses the fact that current X86-64 architecture only implement lower 48-bit virtual address.
    The higher 16-bit can be used for storing other data.
*/
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define SETPOINTER(type, p, x) (p = reinterpret_cast<type *>((reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(0xFFFF000000000000UL)) | reinterpret_cast<uintptr_t>(reinterpret_cast<const void*>(x))))
#define GETPOINTER(type, p) (reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(0x0000FFFFFFFFFFFFUL)))
#else
#error EVT can only be compiled in X86-64 architecture
#endif

namespace evt { namespace chain {

using namespace evt::chain::contracts;
using read_fungible_func = std::function<bool(const asset&)>;
using read_prodvote_func = std::function<bool(const public_key_type& pkey, int64_t value)>;
using read_token_func    = std::function<bool(const token_def& token)>;

class token_database : boost::noncopyable {
public:
    struct flag {
    public:
        flag(uint8_t type, uint8_t op) : type(type), op(op) {}

    public:
        char     payload[6];
        uint8_t  type;
        uint8_t  op;
    };

    // realtime action
    // stored in memory
    struct rt_action {
    public:
        rt_action(uint8_t type, uint8_t op, void* data)
            : f(type, op) {
            SETPOINTER(void, this->data, data);
        }

        union {
            flag  f;
            void* data;
        };
    };

    struct rt_group {
        const void*            rb_snapshot;
        std::vector<rt_action> actions;
    };

    // persistent action
    // stored in disk
    struct pd_action {
        uint16_t    op;
        uint16_t    type;
        std::string key;
        std::string value;
    };

    struct pd_group {
        int64_t                seq; // used for persistent
        std::vector<pd_action> actions;
    };

    struct sp_node {
    public:
        sp_node(uint8_t type) : f(type, 0) {}

    public:
        union {
            flag  f;
            void* group;
        };
    };

    struct savepoint {
    public:
        savepoint(int64_t seq, uint8_t type)
            : seq(seq), node(type) {}

    public:
        int64_t  seq;
        sp_node  node;
    };

public:
    class session {
    public:
        session(token_database& token_db, int seq)
            : _token_db(token_db)
            , _seq(seq)
            , _accept(0) {}
        session(const session& s) = delete;
        session(session&& s) noexcept
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
        void squash() { _accept = 1; _token_db.squash(); }
        void undo()   { _accept = 1; _token_db.rollback_to_latest_savepoint(); }

        int seq() const { return _seq; }

    public:
        session&
        operator=(session&& rhs) noexcept {
            *this = std::move(rhs);
            return *this;
        }

    private:
        token_database& _token_db;
        int             _seq;
        int             _accept;
    };

public:
    token_database();
    token_database(const fc::path& dbpath);
    ~token_database();

public:
    int open(int load_persistence = true);
    int close(int persist = true);

public:
    int add_domain(const domain_def&);
    int exists_domain(const domain_name&) const;
    int issue_tokens(const issuetoken&);
    int exists_token(const domain_name&, const token_name&) const;
    int add_group(const group_def&);
    int exists_group(const group_name&) const;
    int add_suspend(const suspend_def&);
    int exists_suspend(const proposal_name&) const;
    int add_fungible(const fungible_def&);
    int exists_fungible(const symbol) const;
    int exists_fungible(const symbol_id_type) const;
    int exists_lock(const proposal_name&) const;
    int add_lock(const lock_def&);

    int update_asset(const address& addr, const asset&);
    int exists_any_asset(const address& addr) const;
    int exists_asset(const address& addr, const symbol) const;

    int update_prodvote(const conf_key& key, const public_key_type& pkey, int64_t value);

    int add_evt_link(const evt_link_object& link_obj);
    int exists_evt_link(const link_id_type& id) const;

    int read_domain(const domain_name&, domain_def&) const;
    int read_token(const domain_name&, const token_name&, token_def&) const;
    int read_tokens(const domain_name&, int skip, const read_token_func&) const;
    int read_group(const group_name&, group_def&) const;
    int read_suspend(const proposal_name&, suspend_def&) const;
    int read_lock(const proposal_name&, lock_def&) const;

    int read_fungible(const symbol, fungible_def&) const;
    int read_fungible(const symbol_id_type, fungible_def&) const;
    int read_asset(const address& addr, const symbol, asset&) const;
    // this function returns asset(0, symbol) when there's no asset key in address
    // instead of throwing an exception
    int read_asset_no_throw(const address& addr, const symbol, asset&) const;
    int read_all_assets(const address& addr, const read_fungible_func&) const;

    int read_prodvotes_no_throw(const conf_key& key, const read_prodvote_func&) const;

    int read_evt_link(const link_id_type& id, evt_link_object& link_obj) const;

    int update_domain(const domain_def&);
    int update_group(const group_def&);
    int update_token(const token_def&);
    int update_suspend(const suspend_def&);
    int update_fungible(const fungible_def&);
    int update_lock(const lock_def&);

public:
    int add_savepoint(int64_t seq);
    int rollback_to_latest_savepoint();
    int pop_savepoints(int64_t until);
    int pop_back_savepoint();
    int squash();

    int64_t latest_savepoint_seq() const;

    session new_savepoint_session(int64_t seq);
    session new_savepoint_session();

    size_t savepoints_size() const { return savepoints_.size(); }

private:
    int rollback_rt_group(rt_group*);
    int rollback_pd_group(pd_group*);

private:
    int should_record() { return !savepoints_.empty(); }
    int record(uint8_t type, uint8_t op, void* data);
    int free_savepoint(savepoint&);
    int free_all_savepoints();

private:
    int persist_savepoints() const;
    int load_savepoints();
    int persist_savepoints(std::ostream&) const;
    int load_savepoints(std::istream&);

private:
    std::string db_path_;

    rocksdb::DB*          db_;
    rocksdb::ReadOptions  read_opts_;
    rocksdb::WriteOptions write_opts_;

    rocksdb::ColumnFamilyHandle* tokens_handle_;
    rocksdb::ColumnFamilyHandle* assets_handle_;  

    std::deque<savepoint> savepoints_;

    std::unique_ptr<class token_database_impl> my_;

private:
    friend class token_database_impl;
    friend class token_database_snapshot;
};

}}  // namespace evt::chain
