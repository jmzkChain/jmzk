/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <algorithm>
#include <jmzk/chain/block.hpp>
#include <jmzk/chain/merkle.hpp>
#include <fc/bitutil.hpp>
#include <fc/io/raw.hpp>

namespace jmzk { namespace chain {

digest_type
block_header::digest() const {
    return digest_type::hash(*this);
}

uint32_t
block_header::num_from_id(const block_id_type& id) {
    return fc::endian_reverse_u32(id._hash[0]);
}

block_id_type
block_header::id() const {
    // Do not include signed_block_header attributes in id, specifically exclude producer_signature.
    block_id_type result = digest();  // fc::sha256::hash(*static_cast<const block_header*>(this));
    result._hash[0] &= 0xffffffff00000000;
    result._hash[0]
        += fc::endian_reverse_u32(block_num());  // store the block num in the ID, 160 bits is plenty for the hash
    return result;
}

}}  // namespace jmzk::chain
