/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
*/
#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <boost/noncopyable.hpp>
#include <boost/signals2/signal.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/filesystem.hpp>
#include <jmzk/chain/types.hpp>
#include <jmzk/chain/address.hpp>
#include <jmzk/chain/asset.hpp>
#include <jmzk/chain/config.hpp>

namespace rocksdb {
class DB;
class Slice;
}  // namespace rocksdb

namespace jmzk { namespace chain {

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
    jmzklink,
    psvbonus,
    psvbonus_dist,
    validator,
    stakepool,
    script,
    max_value = script
};

enum class action_op {
    add = 0,
    update,
    put
};

struct db_value {
private:
    static const int kInStakeSize = 1024 * 4;
    enum { kArray = 0, kString };
    using data_variant_t = std::variant<std::array<char, kInStakeSize>, std::string>;

public:
    db_value(const db_value& lhs) : var_(lhs.var_) {
        set_view();
    }

    db_value(db_value&& lhs) noexcept : var_(std::move(lhs.var_)) {
        set_view();
    }

private:
    template<typename T>
    db_value(const T& v) : var_(std::in_place_index_t<kArray>()) {
        auto sz = fc::raw::pack_size(v);
        if(sz <= kInStakeSize) {
            auto& arr = std::get<kArray>(var_);
            auto  ds  = fc::datastream<char*>(arr.data(), arr.size());
            fc::raw::pack(ds, v);

            view_ = std::string_view(arr.data(), sz);
        }
        else {
            auto& str = var_.emplace<kString>();
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
    size_t size() const { return view_.size(); }

private:
    data_variant_t   var_;
    std::string_view view_;

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
        storage_profile profile           = storage_profile::disk;
        uint32_t        block_cache_size  = 256 * 1024 * 1024; // 256M
        uint32_t        object_cache_size = 256 * 1024 * 1024; // 256M
        fc::path        db_path           = ::jmzk::chain::config::default_token_database_dir_name;
        bool            enable_stats      = true;
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
    void put_asset(const address& addr, const symbol_id_type sym_id, const std::string_view& data);

    int exists_token(token_type type, const std::optional<name128>& domain, const name128& key) const;
    int exists_asset(const address& addr, const symbol_id_type sym_id) const;

    int read_token(token_type type, const std::optional<name128>& domain, const name128& key, std::string& out, bool no_throw = false) const;
    int read_asset(const address& addr, const symbol_id_type sym_id, std::string& out, bool no_throw = false) const;

    int read_tokens_range(token_type type, const std::optional<name128>& domain, int skip, const read_value_func& func) const;
    int read_assets_range(const symbol_id_type sym_id, int skip, const read_value_func& func) const;

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

public:
    std::string stats() const;

private:
    void flush() const;
    void persist_savepoints(std::ostream&) const;
    void load_savepoints(std::istream&);

private:  // for cache usage
    std::string get_db_key(token_type type, const std::optional<name128>& domain, const name128& key);
    boost::signals2::signal<void(const rocksdb::Slice&)> rollback_token_value;
    boost::signals2::signal<void(const rocksdb::Slice&)> remove_token_value;

private:
    std::unique_ptr<class token_database_impl> my_;
    friend class token_database_cache;
    friend class token_database_impl;
};

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::token_database::config, (profile)(block_cache_size)(object_cache_size)(db_path));
