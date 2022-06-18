
/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/chain/block_timestamp.hpp>
#include <jmzk/chain/types.hpp>
#include <jmzk/chain/multi_index_includes.hpp>

namespace jmzk { namespace chain {

class reversible_block_object : public chainbase::object<reversible_block_object_type, reversible_block_object> {
    OBJECT_CTOR(reversible_block_object, (packedblock))

    id_type       id;
    uint32_t      blocknum = 0;
    shared_string packedblock;

    void
    set_block(const signed_block_ptr& b) {
        packedblock.resize(fc::raw::pack_size(*b));
        fc::datastream<char*> ds(packedblock.data(), packedblock.size());
        fc::raw::pack(ds, *b);
    }

    signed_block_ptr
    get_block() const {
        fc::datastream<const char*> ds(packedblock.data(), packedblock.size());
        auto                        result = std::make_shared<signed_block>();
        fc::raw::unpack(ds, *result);
        return result;
    }
};

struct by_num;
using reversible_block_index = chainbase::shared_multi_index_container<
    reversible_block_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<reversible_block_object, reversible_block_object::id_type, &reversible_block_object::id>>,
        ordered_unique<tag<by_num>, member<reversible_block_object, uint32_t, &reversible_block_object::blocknum>>>>;

struct reversible_block {
    uint32_t blocknum = 0;
    string   packedblock;
};

}}  // namespace jmzk::chain

CHAINBASE_SET_INDEX_TYPE(jmzk::chain::reversible_block_object, jmzk::chain::reversible_block_index);
FC_REFLECT(jmzk::chain::reversible_block, (blocknum)(packedblock));
