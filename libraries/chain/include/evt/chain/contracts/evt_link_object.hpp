/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <fc/io/raw.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts {

struct evt_link_object {
    link_id_type        link_id;
    uint32_t            block_num;
    transaction_id_type trx_id;
};

}}}  // namespace evt::chain::contracts

FC_REFLECT(evt::chain::contracts::evt_link_object, (block_num)(link_id)(trx_id));
