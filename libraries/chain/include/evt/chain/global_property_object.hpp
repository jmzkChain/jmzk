/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include "multi_index_includes.hpp"
#include <chainbase/chainbase.hpp>
#include <jmzk/chain/block_timestamp.hpp>
#include <jmzk/chain/chain_config.hpp>
#include <jmzk/chain/chain_staking_config.hpp>
#include <jmzk/chain/execution_context.hpp>
#include <jmzk/chain/incremental_merkle.hpp>
#include <jmzk/chain/producer_schedule.hpp>
#include <jmzk/chain/staking_context.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

/**
 * @class global_property_object
 * @brief Maintains global state information (committee_member list, current fees)
 * @ingroup object
 * @ingroup implementation
 *
 * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
 */
class global_property_object
    : public chainbase::object<global_property_object_type, global_property_object> {
    OBJECT_CTOR(global_property_object, (proposed_schedule)(action_vers))

    id_type                       id;
    optional<block_num_type>      proposed_schedule_block_num;
    shared_producer_schedule_type proposed_schedule;
    chain_config                  configuration;
    chain_staking_config          staking_configuration;
    staking_context               staking_ctx;
    shared_action_vers            action_vers;
};

/**
 * @class dynamic_global_property_object
 * @brief Maintains global state information (committee_member list, current fees)
 * @ingroup object
 * @ingroup implementation
 *
 * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
 * current values of global blockchain properties.
 */
class dynamic_global_property_object
    : public chainbase::object<dynamic_global_property_object_type, dynamic_global_property_object> {
    OBJECT_CTOR(dynamic_global_property_object)

    id_type              id;
    uint64_t             global_action_sequence = 0;
};

using global_property_multi_index = chainbase::shared_multi_index_container< 
    global_property_object,
    indexed_by<ordered_unique<tag<by_id>,
                              BOOST_MULTI_INDEX_MEMBER(global_property_object, global_property_object::id_type, id)>>>;

using dynamic_global_property_multi_index = chainbase::shared_multi_index_container<
    dynamic_global_property_object,
    indexed_by<ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(dynamic_global_property_object,
                                                                   dynamic_global_property_object::id_type, id)>>>;

}}  // namespace jmzk::chain

CHAINBASE_SET_INDEX_TYPE(jmzk::chain::global_property_object, jmzk::chain::global_property_multi_index);
CHAINBASE_SET_INDEX_TYPE(jmzk::chain::dynamic_global_property_object, jmzk::chain::dynamic_global_property_multi_index);

FC_REFLECT(jmzk::chain::dynamic_global_property_object, (global_action_sequence));
FC_REFLECT(jmzk::chain::global_property_object, (proposed_schedule_block_num)(proposed_schedule)(configuration)(staking_configuration)(staking_ctx)(action_vers));
