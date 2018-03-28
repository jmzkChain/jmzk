/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/evt_contract.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/contracts/chain_initializer.hpp>

#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/producer_object.hpp>

#include <eosio/chain/contracts/abi_serializer.hpp>

#include <eosio/chain/rate_limiting.hpp>

namespace eosio { namespace chain { namespace contracts {

void
apply_eosio_newdomain(apply_context& context) {
    auto ndact = context.act.data_as<newdomain>();
}

void
apply_eosio_issuetoken(apply_context& context) {
    auto itact = context.act.data_as<issuetoken>();
}

void
apply_eosio_transfertoken(apply_context& context) {
    auto ttact = context.act.data_as<transfertoken>();
}

void
apply_eosio_updategroup(apply_context& context) {
    auto ugact = context.act.data_as<updategroup>();
}

} } } // namespace eosio::chain::contracts
