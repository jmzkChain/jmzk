/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/contracts/genesis_state.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/chain_controller.hpp>

namespace evt { namespace chain {  namespace contracts {

   class chain_initializer 
   {

      public:
         chain_initializer(const genesis_state_type& genesis) : genesis(genesis) {}

         time_point              get_chain_start_time();
         chain::chain_config     get_chain_start_configuration();
         producer_schedule_type  get_chain_start_producers();

         void register_types(chain::chain_controller& chain, chainbase::database& db);

         void prepare_database(chain::chain_controller& chain, chainbase::database& db);
         void prepare_tokendb(chain::chain_controller& chain, evt::chain::tokendb& tokendb);

         static abi_def evt_contract_abi();

      private:
         genesis_state_type genesis;
   };

} } } // namespace evt::chain::contracts

