/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <vector>

#include <chainbase/chainbase.hpp>

#include <fc/container/flat.hpp>
#include <fc/container/flat_fwd.hpp>
#include <fc/crypto/private_key.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/fixed_string.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/varint.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/static_variant.hpp>
#include <fc/string.hpp>
#include <fc/uint128.hpp>

#include <evt/chain/name.hpp>

#define OBJECT_CTOR1(NAME)                                   \
    NAME() = delete;                                         \
                                                             \
public:                                                      \
    template <typename Constructor, typename Allocator>      \
    NAME(Constructor&& c, chainbase::allocator<Allocator>) { \
        c(*this);                                            \
    }
#define OBJECT_CTOR2_MACRO(x, y, field) , field(a)
#define OBJECT_CTOR2(NAME, FIELDS)                                     \
    NAME() = delete;                                                   \
                                                                       \
public:                                                                \
    template <typename Constructor, typename Allocator>                \
    NAME(Constructor&& c, chainbase::allocator<Allocator> a)           \
        : id(0) BOOST_PP_SEQ_FOR_EACH(OBJECT_CTOR2_MACRO, _, FIELDS) { \
        c(*this);                                                      \
    }
#define OBJECT_CTOR(...)                        \
    BOOST_PP_OVERLOAD(OBJECT_CTOR, __VA_ARGS__) \
    (__VA_ARGS__)

namespace evt { namespace chain {
using std::all_of;
using std::deque;
using std::enable_shared_from_this;
using std::forward;
using std::make_pair;
using std::map;
using std::move;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::tie;
using std::to_string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::weak_ptr;

using fc::enum_type;
using fc::flat_map;
using fc::flat_set;
using fc::optional;
using fc::path;
using fc::safe;
using fc::signed_int;
using fc::smart_ref;
using fc::static_variant;
using fc::time_point;
using fc::time_point_sec;
using fc::unsigned_int;
using fc::variant;
using fc::variant_object;
using fc::ecc::commitment_type;
using fc::ecc::range_proof_info;
using fc::ecc::range_proof_type;

using public_key_type  = fc::crypto::public_key;
using private_key_type = fc::crypto::private_key;
using signature_type   = fc::crypto::signature;

struct void_t {};

using chainbase::allocator;
using shared_string = boost::interprocess::basic_string<char, std::char_traits<char>, allocator<char>>;
template <typename T>
using shared_vector = boost::interprocess::vector<T, allocator<T>>;
template <typename T>
using shared_set = boost::interprocess::set<T, std::less<T>, allocator<T>>;

using chain_id_type = fc::sha256;

using action_name     = name;
using scope_name      = name;
using permission_name = name;
using domain_name     = name128;
using domain_key      = __uint128_t;
using token_name      = name128;
using account_name    = name128;

/**
 * List all object types from all namespaces here so they can
 * be easily reflected and displayed in debug output.  If a 3rd party
 * wants to extend the core code then they will have to change the
 * packed_object::type field from enum_type to uint16 to avoid
 * warnings when converting packed_objects to/from json.
 */
enum object_type {
    null_object_type,
    account_object_type,
    permission_object_type,
    permission_usage_object_type,
    permission_link_object_type,
    action_code_object_type,
    key_value_object_type,
    key128x128_value_object_type,
    key64x64_value_object_type,
    index64_object_type,
    index128_object_type,
    index256_object_type,
    index_double_object_type,
    action_permission_object_type,
    global_property_object_type,
    dynamic_global_property_object_type,
    block_summary_object_type,
    transaction_object_type,
    generated_transaction_object_type,
    producer_object_type,
    chain_property_object_type,
    account_control_history_object_type,      ///< Defined by account_history_plugin
    account_transaction_history_object_type,  ///< Defined by account_history_plugin
    transaction_history_object_type,          ///< Defined by account_history_plugin
    public_key_history_object_type,           ///< Defined by account_history_plugin
    balance_object_type,                      ///< Defined by native_contract library
    staked_balance_object_type,               ///< Defined by native_contract library
    producer_votes_object_type,               ///< Defined by native_contract library
    producer_schedule_object_type,            ///< Defined by native_contract library
    proxy_vote_object_type,                   ///< Defined by native_contract library
    key64x64x64_value_object_type,
    keystr_value_object_type,
    scope_sequence_object_type,
    bandwidth_usage_object_type,
    compute_usage_object_type,
    table_id_object_type,
    OBJECT_TYPE_COUNT  ///< Sentry value which contains the number of different object types
};

class account_object;
class producer_object;

using block_id_type       = fc::sha256;
using checksum_type       = fc::sha256;
using checksum256_type    = fc::sha256;
using checksum512_type    = fc::sha512;
using checksum160_type    = fc::ripemd160;
using transaction_id_type = checksum_type;
using digest_type         = checksum_type;
using weight_type         = uint16_t;
using block_num_type      = uint32_t;
using share_type          = int64_t;
using int128_t            = __int128_t;
using uint128_t           = __uint128_t;
using bytes               = vector<char>;

/**
 *  Extentions are prefixed with type and are a buffer that can be
 *  interpreted by code that is aware and ignored by unaware code.
 */
typedef vector<std::pair<uint16_t, vector<char>>> extensions_type;

}}  // namespace evt::chain

namespace std {

inline std::ostream&
operator<<(std::ostream& o, const __uint128_t v) {
    const uint64_t P10_UINT64 = 10000000000000000000ULL; /* 19 zeroes */

    if(v > std::numeric_limits<uint64_t>::max()) {
        __uint128_t leading  = v / P10_UINT64;
        uint64_t    trailing = v % P10_UINT64;
        o << leading;
        o << trailing;
    }
    else {
        uint64_t u64 = v;
        o << u64;
    }
    return o;
}

}  // namespace std

FC_REFLECT_ENUM(
    evt::chain::object_type,
    (null_object_type)(account_object_type)(permission_object_type)(permission_usage_object_type)
    (permission_link_object_type)(action_code_object_type)(key_value_object_type)(key128x128_value_object_type)
    (key64x64_value_object_type)(index64_object_type)(index128_object_type)(index256_object_type)
    (index_double_object_type)(action_permission_object_type)(global_property_object_type)
    (dynamic_global_property_object_type)(block_summary_object_type)(transaction_object_type)
    (generated_transaction_object_type)(producer_object_type)(chain_property_object_type)
    (account_control_history_object_type)(account_transaction_history_object_type)(transaction_history_object_type)
    (public_key_history_object_type)(balance_object_type)(staked_balance_object_type)(producer_votes_object_type)
    (producer_schedule_object_type)(proxy_vote_object_type)(key64x64x64_value_object_type)(keystr_value_object_type)
    (scope_sequence_object_type)(bandwidth_usage_object_type)(compute_usage_object_type)(table_id_object_type)
    (OBJECT_TYPE_COUNT))
FC_REFLECT(evt::chain::void_t, )
