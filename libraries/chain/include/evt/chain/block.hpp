/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/block_header.hpp>
#include <evt/chain/transaction.hpp>

namespace evt { namespace chain {

/**
 * When a transaction is referenced by a block it could imply one of several
 * outcomes which describe the state-transition undertaken by the block
 * producer.
 */

struct transaction_receipt_header {
    enum status_enum {
        executed  = 0,  ///< succeed, no error handler executed
        soft_fail = 1,  ///< objectively failed (not executed), error handler executed
        hard_fail = 2,  ///< objectively failed and error handler objectively
                        ///< failed thus no state change
        delayed = 3,    ///< transaction delayed/deferred/scheduled for future execution
        expired = 4     ///< transaction expired and storage space refuned to user
    };

    transaction_receipt_header() : status(hard_fail) {}
    transaction_receipt_header(status_enum s) : status(s) {}

    fc::enum_type<uint8_t, status_enum> status;
};

struct transaction_receipt : public transaction_receipt_header {
    transaction_receipt() : transaction_receipt_header() {}
    transaction_receipt(packed_transaction ptrx) : transaction_receipt_header(executed), trx(ptrx) {}

    packed_transaction trx;

    digest_type
    digest() const {
        digest_type::encoder enc;
        fc::raw::pack(enc, status);
        fc::raw::pack(enc, trx.packed_digest());
        return enc.result();
    }
};

/**
 */
struct signed_block : public signed_block_header {
    using signed_block_header::signed_block_header;
    signed_block() = default;
    signed_block(const signed_block_header& h) : signed_block_header(h) {}

    vector<transaction_receipt> transactions;  /// new or generated transactions
    extensions_type             block_extensions;
};
using signed_block_ptr = std::shared_ptr<signed_block>;

struct producer_confirmation {
    block_id_type  block_id;
    digest_type    block_digest;
    account_name   producer;
    signature_type sig;
};

}}  // namespace evt::chain

FC_REFLECT_ENUM(evt::chain::transaction_receipt::status_enum, (executed)(soft_fail)(hard_fail)(delayed)(expired))

FC_REFLECT(evt::chain::transaction_receipt_header, (status))
FC_REFLECT_DERIVED(evt::chain::transaction_receipt, (evt::chain::transaction_receipt_header), (trx))
FC_REFLECT_DERIVED(evt::chain::signed_block, (evt::chain::signed_block_header), (transactions)(block_extensions))
