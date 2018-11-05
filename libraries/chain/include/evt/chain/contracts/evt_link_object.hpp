/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fc/io/raw.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <evt/chain/multi_index_includes.hpp>

namespace evt { namespace chain { namespace contracts {

using boost::multi_index_container;
using namespace boost::multi_index;

class evt_link_object : public chainbase::object<evt_link_object_type, evt_link_object> {
    OBJECT_CTOR(evt_link_object)

    id_type             id;
    uint32_t            block_num;
    link_id_type        link_id;
    transaction_id_type trx_id;
};  // size: 64

struct by_link_id;
struct by_link_trx_id;
using evt_link_multi_index = chainbase::shared_multi_index_container<
    evt_link_object,
    indexed_by<
        ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(evt_link_object, evt_link_object::id_type, id)>,
        ordered_unique<tag<by_link_id>, BOOST_MULTI_INDEX_MEMBER(evt_link_object, uint128_t, link_id)>,
        ordered_unique<tag<by_link_trx_id>, BOOST_MULTI_INDEX_MEMBER(evt_link_object, transaction_id_type, trx_id)>>>;

typedef chainbase::generic_index<evt_link_multi_index> evt_link_index;

}}}  // namespace evt::chain::contracts

CHAINBASE_SET_INDEX_TYPE(evt::chain::contracts::evt_link_object, evt::chain::contracts::evt_link_multi_index);
FC_REFLECT(evt::chain::evt_link_object, (block_num)(link_id)(trx_id));
