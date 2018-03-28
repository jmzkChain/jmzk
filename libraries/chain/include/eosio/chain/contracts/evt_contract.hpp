/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace contracts { 

    /**
        * @defgroup native_action_handlers Native Action Handlers
        */
    ///@{

        void apply_eosio_newdomain(apply_context& context);
        void apply_eosio_issuetoken(apply_context& context);
        void apply_eosio_transfertoken(apply_context& context);
        void apply_eosio_updategroup(apply_context& context);

    ///@}  end action handlers

} } } /// namespace eosio::contracts
