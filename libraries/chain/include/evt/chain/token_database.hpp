/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>

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

using read_value_func = std::function<int(const std::string_view& key, std::string&&)>;

enum class storage_profile {
    disk   = 0,
    memory = 1
};

enum class action_type {
    asset = 0,
    domain,
    token,
    group,
    suspend,
    lock,
    fungible,
    prodvote,
    evtlink
};

class token_database : boost::noncopyable {
public:
    struct config {
        storage_profile profile;
        uint32_t        cache_size; // MBytes
        std::string     db_path;
    };

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
            if(this == &rhs) {
                return *this;
            }
            *this = std::move(rhs);
            return *this;
        }

    private:
        token_database& _token_db;
        int             _seq;
        int             _accept;
    };

public:
    token_database(const config&);
    ~token_database();

public:
    void open(int load_persistence = true);
    void close(int persist = true);

public:
    void add_token(action_type type, const name128& prefix, const name128& key, const std::string& data);
    void update_token(action_type type, const name128& prefix, const name128& key, const std::string& data);
    void put_token(action_type type, const name128& prefix, const name128& key, const std::string& data);
    void add_tokens(action_type type, const name128& prefix, small_vector<name128, 4>&& keys, const small_vector<std::reference_wrapper<std::string>, 4>& data);
    void put_asset(const address& addr, const symbol sym, const std::string& data);

    int exists_token(const name128& prefix, const name128& key) const;
    int exists_asset(const address& addr, const symbol sym) const;

    int read_token(const name128& prefix, const name128& key, std::string& out, bool no_throw = false) const;
    int read_asset(const address& addr, const symbol sym, std::string& out, bool no_throw = false) const;

    int read_tokens_range(const name128& prefix, int skip, const read_value_func& func) const;
    int read_assets_range(const symbol sym, int skip, const read_value_func& func) const;

public:
    void add_savepoint(int64_t seq);
    void rollback_to_latest_savepoint();
    void pop_savepoints(int64_t until);
    void pop_back_savepoint();
    void squash();

    int64_t latest_savepoint_seq() const;

    session new_savepoint_session(int64_t seq);
    session new_savepoint_session();

    size_t savepoints_size() const;

private:
    void flush() const;
    void persist_savepoints(std::ostream&) const;
    void load_savepoints(std::istream&);

    rocksdb::DB* internal_db() const;
    std::string  get_db_path() const;

private:
    std::unique_ptr<class token_database_impl> my_;

private:
    friend class token_database_snapshot;
};

}}  // namespace evt::chain
