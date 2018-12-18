/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/chain_id_type.hpp>

#include <fc/variant.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

void
chain_id_type::reflector_verify() const {
    EVT_ASSERT(*reinterpret_cast<const fc::sha256*>(this) != fc::sha256(), chain_id_type_exception, "chain_id_type cannot be zero");
}

}}  // namespace evt::chain

namespace fc {

void
to_variant(const evt::chain::chain_id_type& cid, fc::variant& v) {
    to_variant(static_cast<const fc::sha256&>(cid), v);
}

void
from_variant(const fc::variant& v, evt::chain::chain_id_type& cid) {
    from_variant(v, static_cast<fc::sha256&>(cid));
}

}  // namespace fc