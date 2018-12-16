/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <numeric>
#include <evt/chain/action.hpp>
#include <evt/chain/address.hpp>

namespace evt { namespace chain {

/**
 *  The transaction header contains the fixed-sized data
 *  associated with each transaction. It is separated from
 *  the transaction body to facilitate partial parsing of
 *  transactions without requiring dynamic memory allocation.
 *
 *  All transactions have an expiration time after which they
 *  may no longer be included in the blockchain. Once a block
 *  with a block_header::timestamp greater than expiration is
 *  deemed irreversible, then a user can safely trust the transaction
 *  will never be included.
 *

 *  Each region is an independent blockchain, it is included as routing
 *  information for inter-blockchain communication. A contract in this
 *  region might generate or authorize a transaction intended for a foreign
 *  region.
 */
struct transaction_header {
    time_point_sec   expiration;              ///< the time at which a transaction expires
    uint16_t         ref_block_num    = 0U;   ///< specifies a block num in the last 2^16 blocks.
    uint32_t         ref_block_prefix = 0UL;  ///< specifies the lower 32 bits of the blockid at get_ref_blocknum
    uint32_t         max_charge       = 0;    ///< upper limit on the total charge billed for this transaction

    /**
     * @return the absolute block number given the relative ref_block_num
     */
    block_num_type
    get_ref_blocknum(block_num_type head_blocknum) const {
        return ((head_blocknum / 0xffff) * 0xffff) + head_blocknum % 0xffff;
    }
    void set_reference_block(const block_id_type& reference_block);
    bool verify_reference_block(const block_id_type& reference_block) const;
    void validate() const;
};

enum class transaction_ext : uint16_t {
    suspend_name = 0,
    max_value = 0
};

/**
 *  A transaction consits of a set of messages which must all be applied or
 *  all are rejected. These messages have access to data within the given
 *  read and write scopes.
 */
struct transaction : public transaction_header {
    fc::small_vector<action, 2> actions;
    address                     payer;
    extensions_type             transaction_extensions;

    transaction_id_type id() const;
    digest_type         sig_digest(const chain_id_type& chain_id) const;
    public_keys_type    get_signature_keys(const signatures_base_type& signatures,
                                           const chain_id_type&        chain_id,
                                           bool                        allow_duplicate_keys = false) const;

    uint32_t
    total_actions() const {
        return actions.size();
    }
};

struct signed_transaction : public transaction {
    signed_transaction() = default;
    signed_transaction(const transaction& trx, const signatures_type& signatures)
        : transaction(trx)
        , signatures(signatures) {}
    signed_transaction(transaction&& trx, const signatures_type& signatures)
        : transaction(std::move(trx))
        , signatures(signatures) {}

    signatures_type signatures;

    const signature_type& sign(const private_key_type& key, const chain_id_type& chain_id);
    signature_type        sign(const private_key_type& key, const chain_id_type& chain_id) const;
    public_keys_type      get_signature_keys(const chain_id_type& chain_id,
                                             bool                 allow_duplicate_keys = false) const;
};

struct packed_transaction {
    enum compression_type {
        none = 0,
        zlib = 1,
    };

    packed_transaction() = default;

    explicit packed_transaction(const transaction& t, compression_type _compression = none) {
        set_transaction(t, _compression);
    }

    explicit packed_transaction(const signed_transaction& t, compression_type _compression = none)
        : signatures(t.signatures) {
        set_transaction(t, _compression);
    }

    explicit packed_transaction(signed_transaction&& t, compression_type _compression = none)
        : signatures(std::move(t.signatures)) {
        set_transaction(t, _compression);
    }

    uint32_t get_unprunable_size() const;
    uint32_t get_prunable_size() const;

    digest_type packed_digest() const;

    signatures_type                          signatures;
    fc::enum_type<uint8_t, compression_type> compression;
    bytes                                    packed_trx;

    time_point_sec      expiration() const;
    transaction_id_type id() const;
    transaction_id_type get_uncached_id() const; // thread safe
    bytes               get_raw_transaction() const; // thread safe
    transaction         get_transaction() const;
    signed_transaction  get_signed_transaction() const;
    void                set_transaction(const transaction& t, compression_type _compression = none);

private:
    mutable optional<transaction> unpacked_trx;  // <-- intermediate buffer used to retrieve values
    void                          local_unpack() const;
};

using packed_transaction_ptr = std::shared_ptr<packed_transaction>;

uint128_t transaction_id_to_sender_id(const transaction_id_type& tid);

}}  // namespace evt::chain

FC_REFLECT(evt::chain::transaction_header, (expiration)(ref_block_num)(ref_block_prefix)(max_charge));
FC_REFLECT_ENUM(evt::chain::transaction_ext, (suspend_name));
FC_REFLECT_DERIVED(evt::chain::transaction, (evt::chain::transaction_header), (actions)(payer)(transaction_extensions));
FC_REFLECT_DERIVED(evt::chain::signed_transaction, (evt::chain::transaction), (signatures));
FC_REFLECT_ENUM(evt::chain::packed_transaction::compression_type, (none)(zlib));
FC_REFLECT(evt::chain::packed_transaction, (signatures)(compression)(packed_trx));
