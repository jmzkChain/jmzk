/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/block.hpp>
#include <evt/chain/block_header_state.hpp>
#include <evt/chain/transaction_metadata.hpp>

namespace evt { namespace chain {

struct block_state : public block_header_state {
    explicit block_state(const block_header_state& cur) : block_header_state(cur) {}
    block_state(const block_header_state& prev, signed_block_ptr b, bool skip_validate_signee = false);
    block_state(const block_header_state& prev, block_timestamp_type when);
    block_state() = default;

    /// weak_ptr prev_block_state....
    signed_block_ptr block;
    bool             validated        = false;
    bool             in_current_chain = false;

    /// this data is redundant with the data stored in block, but facilitates
    /// recapturing transactions when we pop a block
    vector<transaction_metadata_ptr> trxs;
};

using block_state_ptr = std::shared_ptr<block_state>;

}}  // namespace evt::chain

FC_REFLECT_DERIVED(evt::chain::block_state, (evt::chain::block_header_state), (block)(validated)(in_current_chain));
