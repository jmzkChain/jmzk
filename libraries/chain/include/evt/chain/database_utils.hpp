/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <jmzk/chain/types.hpp>
#include <fc/io/raw.hpp>
#include <fc/crypto/base64.hpp>

namespace jmzk { namespace chain {

template <typename... Indices>
class index_set;

template <typename Index>
class index_utils {
public:
    using index_t = Index;

    template <typename F>
    static void
    walk(const chainbase::database& db, F function) {
        auto const& index = db.get_index<Index>().indices();
        const auto& first = index.begin();
        const auto& last  = index.end();
        for(auto itr = first; itr != last; ++itr) {
            function(*itr);
        }
    }

    template <typename Secondary, typename Key, typename F>
    static void
    walk_range(const chainbase::database& db, const Key& begin_key, const Key& end_key, F function) {
        const auto& idx       = db.get_index<Index, Secondary>();
        auto        begin_itr = idx.lower_bound(begin_key);
        auto        end_itr   = idx.lower_bound(end_key);
        for(auto itr = begin_itr; itr != end_itr; ++itr) {
            function(*itr);
        }
    }

    template <typename Secondary, typename Key>
    static size_t
    size_range(const chainbase::database& db, const Key& begin_key, const Key& end_key) {
        const auto& idx       = db.get_index<Index, Secondary>();
        auto        begin_itr = idx.lower_bound(begin_key);
        auto        end_itr   = idx.lower_bound(end_key);
        size_t      res       = 0;
        while(begin_itr != end_itr) {
            res++;
            ++begin_itr;
        }
        return res;
    }

    template <typename F>
    static void
    create(chainbase::database& db, F cons) {
        db.create<typename index_t::value_type>(cons);
    }
};

template <typename Index>
class index_set<Index> {
public:
    static void
    add_indices(chainbase::database& db) {
        db.add_index<Index>();
    }

    template <typename F>
    static void
    walk_indices(F function) {
        function(index_utils<Index>());
    }
};

template <typename FirstIndex, typename... RemainingIndices>
class index_set<FirstIndex, RemainingIndices...> {
public:
    static void
    add_indices(chainbase::database& db) {
        index_set<FirstIndex>::add_indices(db);
        index_set<RemainingIndices...>::add_indices(db);
    }

    template <typename F>
    static void
    walk_indices(F function) {
        index_set<FirstIndex>::walk_indices(function);
        index_set<RemainingIndices...>::walk_indices(function);
    }
};

}}  // namespace jmzk::chain

namespace fc {

// overloads for to/from_variant
template <typename OidType>
void
to_variant(const chainbase::oid<OidType>& oid, variant& v) {
    v = variant(oid._id);
}

template <typename OidType>
void
from_variant(const variant& v, chainbase::oid<OidType>& oid) {
    from_variant(v, oid._id);
}

inline void
to_variant(const blob& b, variant& v) {
    v = variant(base64_encode(b.data.data(), b.data.size()));
}

inline void
from_variant(const variant& v, blob& b) {
    string _s = base64_decode(v.as_string());
    b.data    = std::vector<char>(_s.begin(), _s.end());
}

template<typename T>
void
to_variant(const jmzk::chain::shared_vector<T>& sv, variant& v) {
    to_variant(std::vector<T>(sv.begin(), sv.end()), v);
}

template<typename T>
void from_variant(const variant& v, jmzk::chain::shared_vector<T>& sv) {
    std::vector<T> _v;
    from_variant(v, _v);
    sv = jmzk::chain::shared_vector<T>(_v.begin(), _v.end(), sv.get_allocator());
}

}  // namespace fc

namespace chainbase {
// overloads for OID packing
template <typename DataStream, typename OidType>
DataStream&
operator<<(DataStream& ds, const oid<OidType>& oid) {
    fc::raw::pack(ds, oid._id);
    return ds;
}

template <typename DataStream, typename OidType>
DataStream&
operator>>(DataStream& ds, oid<OidType>& oid) {
    fc::raw::unpack(ds, oid._id);
    return ds;
}

}  // namespace chainbase
