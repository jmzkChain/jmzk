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
    int open(int load_persistence = true);
    int close(int persist = true);

public:
    

public:
    int add_savepoint(int64_t seq);
    int rollback_to_latest_savepoint();
    int pop_savepoints(int64_t until);
    int pop_back_savepoint();
    int squash();

    int64_t latest_savepoint_seq() const;

    session new_savepoint_session(int64_t seq);
    session new_savepoint_session();

    size_t savepoints_size() const;

private:
    int flush() const;
    int persist_savepoints(std::ostream&) const;
    int load_savepoints(std::istream&);

    rocksdb::DB* internal_db() const;
    std::string  get_db_path() const;

private:
    std::unique_ptr<class token_database_impl> my_;

private:
    friend class token_database_snapshot;
};

}}  // namespace evt::chain
