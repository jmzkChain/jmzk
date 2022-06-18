/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <jmzk/chain/types.hpp>

namespace jmzk { namespace chain {

/**
 *  For each action dispatched this receipt is generated
 */
struct action_receipt {
    digest_type act_digest;
    uint64_t    global_sequence = 0;  ///< total number of actions dispatched since genesis

    digest_type digest() const { return digest_type::hash(*this); }
};

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::action_receipt, (act_digest)(global_sequence));
