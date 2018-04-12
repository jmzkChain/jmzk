/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/transaction.hpp>
#include <evt/chain/block.hpp>

namespace evt { namespace chain {

class transaction_metadata {
   public:
      transaction_metadata( const transaction& t, const time_point& published, const char* raw_data, size_t raw_size )
         :id(t.id())
         ,published(published)
         ,raw_data(raw_data),raw_size(raw_size),_trx(&t)
      {}

      transaction_metadata( const transaction& t, const time_point& published, const char* raw_data, size_t raw_size, fc::time_point deadline )
         :id(t.id())
         ,published(published)
         ,raw_data(raw_data),raw_size(raw_size)
         ,processing_deadline(deadline)
         ,_trx(&t)
      {}

      transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published );

      transaction_metadata( transaction_metadata && ) = default;
      transaction_metadata& operator= (transaction_metadata &&) = default;

      // things for packed_transaction
      optional<bytes>                       raw_trx;
      optional<transaction>                 decompressed_trx;
      digest_type                           packed_digest;

      // things for signed/packed transactions
      optional<flat_set<public_key_type>>   signing_keys;

      transaction_id_type                   id;

      uint32_t                              region_id       = 0;
      uint32_t                              cycle_index     = 0;
      uint32_t                              shard_index     = 0;
      uint32_t                              signature_count = 0;
      time_point                            published;

      // packed form to pass to contracts if needed
      const char*                           raw_data = nullptr;
      size_t                                raw_size = 0;

      vector<char>                          packed_trx;

      const transaction& trx() const{
         if (decompressed_trx) {
            return *decompressed_trx;
         } else {
            return *_trx;
         }
      }

      // limits
      optional<time_point>                  processing_deadline;

   private:
      const transaction* _trx = nullptr;
};

} } // evt::chain

FC_REFLECT( evt::chain::transaction_metadata, (raw_trx)(signing_keys)(id)(region_id)(cycle_index)(shard_index)(published) )