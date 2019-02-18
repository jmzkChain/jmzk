/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <memory>
#include <boost/type_index.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>
#include <rocksdb/cache.h>
#include <evt/chain/token_database.hpp>

namespace evt { namespace chain {

class token_database_cache {
public:
    token_database_cache(token_database& db, size_t cache_size) : db_(db) {
        cache_ = rocksdb::NewLRUCache(cache_size);
    }

private:
    struct cache_key {
    public:
        cache_key(token_type type, const std::optional<name128>& domain, const name128& key) {
            auto ds = fc::datastream<char*>(buf_);
            fc::raw::pack(ds, (int)type);
            fc::raw::pack(ds, domain);
            fc::raw::pack(ds, key);

            slice_ = rocksdb::Slice(buf_, ds.tell());
        }

    public:
        const rocksdb::Slice& as_slice() const { return slice_; }

    private:
        char           buf_[sizeof(name128) * 2 + sizeof(int) + 1];
        rocksdb::Slice slice_;
    };

    template<typename T>
    struct cache_entry {
    public:
        cache_entry() : ti(boost::typeindex::type_id<T>()) {}
        cache_entry(T&& d) : ti(boost::typeindex::type_id<T>()), data(std::forward<T>(d)) {}

    public:
        boost::typeindex::type_index ti;
        T                            data;
    };

    struct cache_entry_deleter {
    public:
        cache_entry_deleter(token_database_cache& self, void* handle)
            : self_(self), handle_(handle) {}

    void
    operator()(cache_entry* ptr) {
        self_.cache_->Release(handle_);
    }

    private:
        token_database_cache& self_;
        void*                 handle_;
    };

public:
    template<typename T>
    std::shared_ptr<T>
    read_token(token_type type, const std::optional<name128>& domain, const name128& key, bool no_throw = false) {
        auto k = cache_key(type, domain, key);
        auto h = cache_->Lookup(k.as_slice());
        if(h != nullptr) {
            auto entry = (cache_entry*)cache_->Value(h);
            EVT_ASSERT2(entry->ti == boost::typeindex::type_id<T>(), token_database_cache_exception,
                "Types are not matched between cache({}) and query({})", entry->ti.pretty_name(), boost::typeindex::type_id<T>().pretty_name());
            return std::shared_ptr<T>(&entry.data, cache_entry_deleter(*this, h));
        }

        auto str = std::string();
        auto r   = db_.read_token(type, domain, key, str, no_throw);
        if(no_throw && !r) {
            return nullptr;
        }

        auto entry = new cache_entry();
        extract_db_value(str, entry.data);

        auto s = cache_->Insert(k.as_slice(), (void*)entry, str.size(), [](auto& ck, auto cv) { delete (T*)cv; }, &h);
        FC_ASSERT(s == rocksdb::Status::Ok());

        return std::shared_ptr<T>(&entry.data, cache_entry_deleter(*this, h));
    }

    template<typename T>
    void
    put_token(token_type type, action_op op, const std::optional<name128>& domain, const name128& key, T&& data) {
        auto k = cache_key(type, domain, key);
        FC_ASSERT(!cache_->Lookup(k.as_slice()));

        auto v     = make_db_value(data);
        auto entry = new cache_entry<T>(std::forward<T>(data));

        auto s = cache_->Insert(k.as_slice(), (void*)entry, v.size(), [](auto& ck, auto cv) { delete (T*)cv; }, &h);
        FC_ASSERT(s == rocksdb::Status::Ok());
    }

private:
    token_database&        db_;
    std::shared_ptr<Cache> cache_;
};

}}  // namespace evt::chain
