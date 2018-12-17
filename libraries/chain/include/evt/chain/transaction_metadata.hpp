/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/block.hpp>
#include <evt/chain/trace.hpp>
#include <evt/chain/transaction.hpp>

namespace evt { namespace chain {

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
public:
    transaction_id_type                             id;
    transaction_id_type                             signed_id;
    signed_transaction                              trx;
    packed_transaction                              packed_trx;
    optional<pair<chain_id_type, public_keys_type>> signing_keys;
    bool                                            accepted = false;
    bool                                            implicit = false;

    explicit transaction_metadata(const signed_transaction& t, packed_transaction::compression_type c = packed_transaction::none)
        : trx(t)
        , packed_trx(t, c) {
        id = trx.id();
        // raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
        signed_id = digest_type::hash(packed_trx);
    }

    explicit transaction_metadata(const packed_transaction& ptrx)
        : trx(ptrx.get_signed_transaction())
        , packed_trx(ptrx) {
        id = trx.id();
        // raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
        signed_id = digest_type::hash(packed_trx);
    }

    const public_keys_type&
    recover_keys(const chain_id_type& chain_id) {
        if(!signing_keys.has_value() || signing_keys->first != chain_id) {  // Unlikely for more than one chain_id to be used in one nodeos instance
            signing_keys = std::make_pair(chain_id, trx.get_signature_keys(chain_id));
        }
        return signing_keys->second;
    }

    uint32_t
    total_actions() const {
        return trx.actions.size();
    }
};

using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;

}}  // namespace evt::chain
