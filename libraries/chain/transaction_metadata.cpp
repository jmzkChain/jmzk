/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/transaction_metadata.hpp>
#include <evt/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace evt { namespace chain {

transaction_metadata::transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published )
   :raw_trx(t.get_raw_transaction())
   ,decompressed_trx(fc::raw::unpack<transaction>(*raw_trx))
   ,packed_digest(t.packed_digest())
   ,id(decompressed_trx->id())
   ,signature_count(t.signatures.size())
   ,published(published)
   ,raw_data(raw_trx->data())
   ,raw_size(raw_trx->size())
{ }

} } // evt::chain