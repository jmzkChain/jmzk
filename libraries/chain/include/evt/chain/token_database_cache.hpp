/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
*/
#pragma once
#include <memory>
#include <boost/type_index.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>
#include <rocksdb/cache.h>
#include <jmzk/chain/token_database.hpp>

namespace jmzk { namespace chain {

class token_database_cache {
public:
    token_database_cache(token_database& db, size_t cache_size)
        : db_(db)
        , cache_(rocksdb::NewLRUCache(cache_size)) {
        watch_db();
    }

private:
    template<typename T>
    struct cache_entry {
    public:
        cache_entry() : ti(boost::typeindex::type_id<T>()) {}

        template<typename U>
        cache_entry(U&& d) : ti(boost::typeindex::type_id<T>()), data(std::forward<U>(d)) {}

    public:
        boost::typeindex::type_index ti;
        T                            data;
    };

public:
    template<typename T>
    struct cache_deleter {
    public:
        cache_deleter()
            : self_(nullptr), handle_(nullptr) {}
        cache_deleter(token_database_cache* self, rocksdb::Cache::Handle* handle)
            : self_(self), handle_(handle) {}

    void
    operator()(T* ptr) {
        assert(handle_);
        self_->cache_->Release(handle_);
    }

    private:
        token_database_cache*   self_;
        rocksdb::Cache::Handle* handle_;
    };

public:
    template<typename T>
    std::unique_ptr<T, cache_deleter<T>>
    read_token(token_type type, const std::optional<name128>& domain, const name128& key, bool no_throw = false) {
        static_assert(std::is_class_v<T>, "T should be a class type");

        auto k = db_.get_db_key(type, domain, key);
        auto h = cache_->Lookup(k);
        if(h != nullptr) {
            auto entry = (cache_entry<T>*)cache_->Value(h);
            jmzk_ASSERT2(entry->ti == boost::typeindex::type_id<T>(), token_database_cache_exception,
                "Types are not matched between cache({}) and query({})", entry->ti.pretty_name(), boost::typeindex::type_id<T>().pretty_name());
            return std::unique_ptr<T, cache_deleter<T>>(&entry->data, cache_deleter<T>(this, h));
        }

        auto str = std::string();
        auto r   = db_.read_token(type, domain, key, str, no_throw);
        if(no_throw && !r) {
            return nullptr;
        }

        auto entry = new cache_entry<T>();
        extract_db_value(str, entry->data);

        auto s = cache_->Insert(k, (void*)entry, str.size(),
            [](auto& ck, auto cv) { delete (cache_entry<T>*)cv; }, &h);
        FC_ASSERT(s == rocksdb::Status::OK());

        return std::unique_ptr<T, cache_deleter<T>>(&entry->data, cache_deleter<T>(this, h));
    }

    template<typename T>
    std::unique_ptr<T, cache_deleter<T>>
    lookup_token(token_type type, const std::optional<name128>& domain, const name128& key, bool no_throw = false) {
        static_assert(std::is_class_v<T>, "T should be a class type");

        auto k = db_.get_db_key(type, domain, key);
        auto h = cache_->Lookup(k);
        if(h != nullptr) {
            auto entry = (cache_entry<T>*)cache_->Value(h);
            jmzk_ASSERT2(entry->ti == boost::typeindex::type_id<T>(), token_database_cache_exception,
                "Types are not matched between cache({}) and query({})", entry->ti.pretty_name(), boost::typeindex::type_id<T>().pretty_name());
            return std::unique_ptr<T, cache_deleter<T>>(&entry->data, cache_deleter<T>(this, h));
        }
        return nullptr;
    }

    template<typename T, bool RtnPTR = false, typename U = std::decay_t<T>>
    std::conditional_t<RtnPTR, std::unique_ptr<U, cache_deleter<U>>, void>
    put_token(token_type type, action_op op, const std::optional<name128>& domain, const name128& key, T&& data)  {
        static_assert(std::is_class_v<U>, "Underlying of T should be a class type");
        using entry_t = cache_entry<U>;

        auto k = db_.get_db_key(type, domain, key);
        auto h = cache_->Lookup(k);
        if(h != nullptr) {
            auto entry = (entry_t*)cache_->Value(h);
            jmzk_ASSERT2(entry->ti == boost::typeindex::type_id<T>(), token_database_cache_exception,
                "Types are not matched between cache({}) and query({})", entry->ti.pretty_name(), boost::typeindex::type_id<T>().pretty_name());
            jmzk_ASSERT2(&entry->data == &data, token_database_cache_exception,
                "Provided updated data object should be the same as original one in cache");
        }

        auto v = make_db_value(data);
        db_.put_token(type, op, domain, key, v.as_string_view());
        
        if(h != nullptr) {
            // if there's already cache item, no need to insert new one
            if constexpr(!RtnPTR) {
                return;
            }
            else {
                return nullptr;
            }
        }

        auto entry = new entry_t(std::forward<T>(data));
        if constexpr(!RtnPTR) {
            auto s = cache_->Insert(k, (void*)entry, v.size(),
                [](auto& ck, auto cv) { delete (cache_entry<U>*)cv; }, nullptr /* handle */);
            FC_ASSERT(s == rocksdb::Status::OK());
        }
        else {
            auto s = cache_->Insert(k, (void*)entry, v.size(),
                [](auto& ck, auto cv) { delete (cache_entry<U>*)cv; }, &h);
            FC_ASSERT(s == rocksdb::Status::OK());
            return std::unique_ptr<U, cache_deleter<U>>(&entry->data, cache_deleter<U>(this, h));
        }
    }

private:
    void
    watch_db() {
        db_.rollback_token_value.connect([this](auto& key) {
            cache_->Erase(key);
        });
        db_.remove_token_value.connect([this](auto& key) {
            cache_->Erase(key);
        });
    }

private:
    token_database&                 db_;
    std::shared_ptr<rocksdb::Cache> cache_;
};

template<typename T>
auto make_empty_cache_ptr = [] {
    return std::unique_ptr<T, token_database_cache::cache_deleter<T>>(nullptr);
};

}}  // namespace jmzk::chain
