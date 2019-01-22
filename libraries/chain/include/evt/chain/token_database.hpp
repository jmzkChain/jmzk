/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <boost/noncopyable.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/filesystem.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/address.hpp>
#include <evt/chain/config.hpp>

namespace rocksdb {
class DB;
}  // namespace rocksdb

namespace evt { namespace chain {

using read_value_func = std::function<bool(const std::string_view& key, std::string&&)>;

enum class storage_profile {
    disk   = 0,
    memory = 1
};

enum class token_type {
    asset = 0,
    domain,
    token,
    group,
    suspend,
    lock,
    fungible,
    prodvote,
    evtlink,
    bonus,
    bonuspsvdist,
    max_value = bonuspsvdist
};

enum class action_op {
    add = 0,
    update,
    put
};

struct db_value {
public:
    db_value(const db_value& lhs) : var_(lhs.var_) {
        set_view();
    }

    db_value(db_value&& lhs) noexcept : var_(std::move(lhs.var_)) {
        set_view();
    }

private:
    template<typename T>
    db_value(const T& v) : var_(std::in_place_index_t<0>()) {
        auto sz = fc::raw::pack_size(v);
        if(sz <= 1024 * 1024) {
            auto& arr = std::get<0>(var_);
            auto  ds  = fc::datastream<char*>(arr.data(), arr.size());
            fc::raw::pack(ds, v);

            view_ = std::string_view(arr.data(), sz);
        }
        else {
            auto& str = std::get<1>(var_);
            str.resize(sz);

            auto ds = fc::datastream<char*>((char*)str.data(), str.size());
            fc::raw::pack(ds, v);

            view_ = std::string_view(str.data(), sz);
        }
    }

    void
    set_view() {
        std::visit([this](auto& buf) {
            view_ = std::string_view(buf.data(), buf.size());
        }, var_);
    }

public:
    std::string_view as_string_view() const { return view_; }

private:
    std::variant<std::array<char, 1024 * 4>, std::string> var_;
    std::string_view                                      view_;

public:
    template<typename T>
    friend db_value make_db_value(const T&);
};

template<typename T>
db_value
make_db_value(const T& v) {
    return db_value(v);
}

template<typename T>
void
extract_db_value(const std::string& str, T& v) {
    auto ds = fc::datastream<const char*>(str.data(), str.size());
    fc::raw::unpack(ds, v);
}

using token_keys_t = small_vector<name128, 4>;

class token_database : boost::noncopyable {
public:
    struct config {
        storage_profile profile    = storage_profile::disk;
        uint32_t        cache_size = 256; // MBytes
        fc::path        db_path    = ::evt::chain::config::default_token_database_dir_name;
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
    void put_token(token_type type, action_op op, const std::optional<name128>& domain, const name128& key, const std::string_view& data);
    void put_tokens(token_type type, action_op op, const std::optional<name128>& domain, token_keys_t&& keys, const small_vector_base<std::string_view>& data);
    void put_asset(const address& addr, const symbol sym, const std::string_view& data);

    int exists_token(token_type type, const std::optional<name128>& domain, const name128& key) const;
    int exists_asset(const address& addr, const symbol sym) const;

    int read_token(token_type type, const std::optional<name128>& domain, const name128& key, std::string& out, bool no_throw = false) const;
    int read_asset(const address& addr, const symbol sym, std::string& out, bool no_throw = false) const;

    int read_tokens_range(token_type type, const std::optional<name128>& domain, int skip, const read_value_func& func) const;
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
    fc::path     get_db_path() const;

private:
    std::unique_ptr<class token_database_impl> my_;

private:
    friend class token_database_snapshot;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::token_database::config, (profile)(cache_size)(db_path));
