/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/multi_index_includes.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain {
/**
 *  @brief tracks minimal information about past blocks to implement TaPOS
 *  @ingroup object
 *
 *  When attempting to calculate the validity of a transaction we need to
 *  lookup a past block and check its block hash and the time it occurred
 *  so we can calculate whether the current transaction is valid and at
 *  what time it should expire.
 */
class block_summary_object : public chainbase::object<block_summary_object_type, block_summary_object> {
    OBJECT_CTOR(block_summary_object)

    id_type       id;
    block_id_type block_id;
};

struct by_block_id;
using block_summary_multi_index = chainbase::shared_multi_index_container<
    block_summary_object,
    indexed_by<
        ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_summary_object::id_type, id)>
        //      ordered_unique<tag<by_block_id>, BOOST_MULTI_INDEX_MEMBER(block_summary_object, block_id_type,
        //      block_id)>
        >>;

}}  // namespace evt::chain

CHAINBASE_SET_INDEX_TYPE(evt::chain::block_summary_object, evt::chain::block_summary_multi_index);
FC_REFLECT(evt::chain::block_summary_object, (block_id));
