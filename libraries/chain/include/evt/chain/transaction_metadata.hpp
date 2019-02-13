/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <boost/noncopyable.hpp>
#include <evt/chain/block.hpp>
#include <evt/chain/trace.hpp>
#include <evt/chain/transaction.hpp>

namespace evt { namespace chain {

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata : boost::noncopyable {
public:
    transaction_id_type                             id;
    transaction_id_type                             signed_id;
    packed_transaction_ptr                          packed_trx;
    optional<pair<chain_id_type, public_keys_set>>  signing_keys;
    bool                                            accepted = false;
    bool                                            implicit = false;

public:
    explicit transaction_metadata(const signed_transaction& t, packed_transaction::compression_type c = packed_transaction::none)
        : id(t.id()), packed_trx(std::make_shared<packed_transaction>(t, c)) {
        signed_id = digest_type::hash(*packed_trx);
    }

    explicit transaction_metadata(const packed_transaction_ptr& ptrx)
        : id(ptrx->id()), packed_trx(ptrx) {
        signed_id = digest_type::hash(*packed_trx);
    }

public:
    const public_keys_set&
    recover_keys(const chain_id_type& chain_id) {
        if(!signing_keys.has_value() || signing_keys->first != chain_id) {  // Unlikely for more than one chain_id to be used in one nodeos instance
            signing_keys = std::make_pair(chain_id, packed_trx->get_signed_transaction().get_signature_keys(chain_id));
        }
        return signing_keys->second;
    }
};

using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;

}}  // namespace evt::chain
