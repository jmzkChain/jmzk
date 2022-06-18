/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <fc/io/raw.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain { namespace contracts {

struct jmzk_link_object {
    link_id_type        link_id;
    uint32_t            block_num;
    transaction_id_type trx_id;
};

}}}  // namespace jmzk::chain::contracts

FC_REFLECT(jmzk::chain::contracts::jmzk_link_object, (block_num)(link_id)(trx_id));
