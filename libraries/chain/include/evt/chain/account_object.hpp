/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain/types.hpp>
#include <evt/chain/authority.hpp>
#include <evt/chain/block_timestamp.hpp>
#include <evt/chain/contracts/types.hpp>

#include "multi_index_includes.hpp"

namespace evt { namespace chain {

   class account_object : public chainbase::object<account_object_type, account_object> {
      OBJECT_CTOR(account_object,(abi))

      id_type              id;
      account_name         name;
      bool                 privileged   = false;
      bool                 frozen       = false;

      block_timestamp_type creation_date;

      shared_vector<char>  abi;

      void set_abi( const evt::chain::contracts::abi_def& a ) {
         // Added resize(0) here to avoid bug in boost vector container
         abi.resize( 0 );
         abi.resize( fc::raw::pack_size( a ) );
         fc::datastream<char*> ds( abi.data(), abi.size() );
         fc::raw::pack( ds, a );
      }

      evt::chain::contracts::abi_def get_abi()const {
         evt::chain::contracts::abi_def a;
         fc::datastream<const char*> ds( abi.data(), abi.size() );
         fc::raw::unpack( ds, a );
         return a;
      }
   };
   using account_id_type = account_object::id_type;

   struct by_name;
   using account_index = chainbase::shared_multi_index_container<
      account_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_object, account_object::id_type, &account_object::id>>,
         ordered_unique<tag<by_name>, member<account_object, account_name, &account_object::name>>
      >
   >;

} } // evt::chain

CHAINBASE_SET_INDEX_TYPE(evt::chain::account_object, evt::chain::account_index)


FC_REFLECT(evt::chain::account_object, (name)(creation_date))
