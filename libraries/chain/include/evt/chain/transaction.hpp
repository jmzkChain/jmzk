/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>
#include <evt/chain/config.hpp>

#include <numeric>

namespace evt { namespace chain {

   struct action {
      action_name                name;
      domain_name                domain;
      domain_key                 key;
      bytes                      data;

      action(){}

      template<typename T>
      action( const domain_name& domain, const domain_key& key, const T& value ) 
            : domain(domain), key(key) 
      {
          name = T::get_name();
          data = fc::raw::pack(value);
      }

      action( const action_name name, const domain_name& domain, const domain_key& key, const bytes& data )
            : name(name), domain(domain), key(key), data(data) {
      }

      template<typename T>
      T data_as()const {
         FC_ASSERT( name == T::get_name()  ); 
         return fc::raw::unpack<T>(data);
      }
   };

   struct action_notice : public action {
      account_name receiver;
   };


   /**
    * When a transaction is referenced by a block it could imply one of several outcomes which 
    * describe the state-transition undertaken by the block producer. 
    */
   struct transaction_receipt {
      enum status_enum {
         executed  = 0, ///< succeed, no error handler executed
         soft_fail = 1, ///< objectively failed (not executed), error handler executed
         hard_fail = 2  ///< objectively failed and error handler objectively failed thus no state change
      };

      transaction_receipt() : status(hard_fail) {}
      transaction_receipt( transaction_id_type tid ):status(executed),id(tid){}

      fc::enum_type<uint8_t,status_enum>  status;
      transaction_id_type                 id;
   };

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
      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               region        = 0; ///< the computational memory region this transaction applies to.
      uint16_t               ref_block_num = 0; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix = 0; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum

      /**
       * @return the absolute block number given the relative ref_block_num
       */
      block_num_type get_ref_blocknum( block_num_type head_blocknum )const {
         return ((head_blocknum/0xffff)*0xffff) + head_blocknum%0xffff;
      }
      void set_reference_block( const block_id_type& reference_block );
      bool verify_reference_block( const block_id_type& reference_block )const;
   };

   /**
    *  A transaction consits of a set of messages which must all be applied or
    *  all are rejected. These messages have access to data within the given
    *  read and write scopes.
    */
   struct transaction : public transaction_header {
      vector<action>             actions;

      transaction_id_type        id()const;
      digest_type                sig_digest( const chain_id_type& chain_id )const;
      flat_set<public_key_type>  get_signature_keys( const vector<signature_type>& signatures, const chain_id_type& chain_id )const;

   };

   struct signed_transaction : public transaction
   {
      signed_transaction() = default;
//      signed_transaction( const signed_transaction& ) = default;
//      signed_transaction( signed_transaction&& ) = default;
      signed_transaction( transaction&& trx, const vector<signature_type>& signatures)
      : transaction(std::forward<transaction>(trx))
      , signatures(signatures)
      {
      }

      vector<signature_type>    signatures;

      const signature_type&     sign(const private_key_type& key, const chain_id_type& chain_id);
      signature_type            sign(const private_key_type& key, const chain_id_type& chain_id)const;
      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id )const;
   };

   struct packed_transaction {
      enum compression_type {
         none = 0,
         zlib = 1,
      };

      packed_transaction() = default;

      explicit packed_transaction(const transaction& t, compression_type _compression = none)
      {
         set_transaction(t, _compression);
      }

      explicit packed_transaction(const signed_transaction& t, compression_type _compression = none)
      :signatures(t.signatures)
      {
         set_transaction(t, _compression);
      }

      explicit packed_transaction(signed_transaction&& t, compression_type _compression = none)
      :signatures(std::move(t.signatures))
      {
         set_transaction(t, _compression);
      }

      digest_type packed_digest()const;

      vector<signature_type>                    signatures;
      fc::enum_type<uint8_t, compression_type>  compression;
      bytes                                     packed_trx;

      bytes                     get_raw_transaction()const;
      transaction               get_transaction()const;
      signed_transaction        get_signed_transaction()const;
      void                      set_transaction(const transaction& t, compression_type _compression = none);

   };

} } // evt::chain

FC_REFLECT( evt::chain::action, (name)(domain)(key)(data) )
FC_REFLECT( evt::chain::transaction_header, (expiration)(region)(ref_block_num)(ref_block_prefix) )
FC_REFLECT_DERIVED( evt::chain::transaction, (evt::chain::transaction_header), (actions) )
FC_REFLECT_DERIVED( evt::chain::signed_transaction, (evt::chain::transaction), (signatures) )
FC_REFLECT_ENUM( evt::chain::packed_transaction::compression_type, (none)(zlib) )
FC_REFLECT( evt::chain::packed_transaction, (signatures)(compression)(packed_trx) )
FC_REFLECT( evt::chain::transaction_receipt, (status)(id) )
