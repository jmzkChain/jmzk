/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/chain_id_type.hpp>

#include <fc/variant.hpp>
#include <jmzk/chain/exceptions.hpp>

namespace jmzk { namespace chain {

void
chain_id_type::reflector_init() const {
    jmzk_ASSERT(*reinterpret_cast<const fc::sha256*>(this) != fc::sha256(), chain_id_type_exception, "chain_id_type cannot be zero");
}

}}  // namespace jmzk::chain

namespace fc {

void
to_variant(const jmzk::chain::chain_id_type& cid, fc::variant& v) {
    to_variant(static_cast<const fc::sha256&>(cid), v);
}

void
from_variant(const fc::variant& v, jmzk::chain::chain_id_type& cid) {
    from_variant(v, static_cast<fc::sha256&>(cid));
}

}  // namespace fc