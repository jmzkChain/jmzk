/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/types.hpp>

namespace evt { namespace chain {

/**
    *  For each action dispatched this receipt is generated
    */
struct action_receipt {
    digest_type act_digest;
    uint64_t    global_sequence = 0;  ///< total number of actions dispatched since genesis

    digest_type
    digest() const { return digest_type::hash(*this); }
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::action_receipt, (act_digest)(global_sequence));
