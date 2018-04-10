/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/block_timestamp.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/producer_schedule.hpp>

namespace evt { namespace chain {

   struct block_header
   {
      digest_type     digest() const;
      uint32_t        block_num() const { return num_from_id(previous) + 1; }
      static uint32_t num_from_id(const block_id_type& id);

      block_id_type                     previous;
      block_timestamp_type              timestamp;

      checksum256_type                  transaction_mroot; /// mroot of cycles_summary
      checksum256_type                  action_mroot;
      checksum256_type                  block_mroot;

      account_name                      producer;

      /** The producer schedule version that should validate this block, this is used to
       * indicate that the prior block which included new_producers->version has been marked
       * irreversible and that it the new producer schedule takes effect this block.
       */
      uint32_t                          schedule_version = 0;
      optional<producer_schedule_type>  new_producers;
   };

   struct signed_block_header : public block_header
   {
      block_id_type              id() const;
      public_key_type            signee() const;
      void                       sign(const private_key_type& signer);
      bool                       validate_signee(const public_key_type& expected_signee) const;

      signature_type             producer_signature;
   };

   struct shard_summary {
      vector<transaction_receipt>   transactions; /// new or generated transactions
      bool empty() const {
          return transactions.empty();
      }
   };

   typedef vector<shard_summary>    cycle;

   struct region_summary {
      uint16_t region = 0;
      vector<cycle>    cycles_summary;
   };

   /**
    *  The block_summary defines the set of transactions that were successfully applied as they
    *  are organized into cycles and shards. A shard contains the set of transactions IDs which
    *  are either user generated transactions or code-generated transactions.
    *
    *
    *  The primary purpose of a block is to define the order in which messages are processed.
    *
    *  The secodnary purpose of a block is certify that the messages are valid according to
    *  a group of 3rd party validators (producers).
    *
    *  The next purpose of a block is to enable light-weight proofs that a transaction occured
    *  and was considered valid.
    *
    *  The next purpose is to enable code to generate messages that are certified by the
    *  producers to be authorized.
    *
    *  A block is therefore defined by the ordered set of executed and generated transactions,
    *  and the merkle proof is over set of messages delivered as a result of executing the
    *  transactions.
    *
    *  A message is defined by { receiver, code, function, permission, data }, the merkle
    *  tree of a block should be generated over a set of message IDs rather than a set of
    *  transaction ids.
    */
   struct signed_block_summary : public signed_block_header {
      vector<region_summary>    regions;
   };

   /**
    * This structure contains the set of signed transactions referenced by the
    * block summary. This inherits from block_summary/signed_block_header and is
    * what would be logged to disk to enable the regeneration of blockchain state.
    *
    * The transactions are grouped to mirror the cycles in block_summary, generated
    * transactions are not included.
    */
   struct signed_block : public signed_block_summary {
      signed_block () = default;
      signed_block (const signed_block& ) = default;
      signed_block (const signed_block_summary& base)
         :signed_block_summary (base),
          input_transactions()
      {}

      /// this is loaded and indexed into map<id,trx> that is referenced by summary; order doesn't matter
      vector<packed_transaction>   input_transactions;
   };

} } // evt::chain

FC_REFLECT(evt::chain::block_header, (previous)(timestamp)
           (transaction_mroot)(action_mroot)(block_mroot)
           (producer)(schedule_version)(new_producers))

FC_REFLECT_DERIVED(evt::chain::signed_block_header, (evt::chain::block_header), (producer_signature))
FC_REFLECT( evt::chain::shard_summary, (transactions))
FC_REFLECT( evt::chain::region_summary, (region)(cycles_summary) )
FC_REFLECT_DERIVED(evt::chain::signed_block_summary, (evt::chain::signed_block_header), (regions))
FC_REFLECT_DERIVED(evt::chain::signed_block, (evt::chain::signed_block_summary), (input_transactions))
