/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/apply_context.hpp>
#include <evt/chain/types.hpp>

namespace evt { namespace chain { namespace contracts { 

    /**
        * @defgroup native_action_handlers Native Action Handlers
        */
    ///@{

        void apply_evt_newdomain(apply_context& context);
        void apply_evt_issuetoken(apply_context& context);
        void apply_evt_transfer(apply_context& context);
        void apply_evt_updategroup(apply_context& context);
        void apply_evt_updatedomain(apply_context& context);

        void apply_evt_newaccount(apply_context& context);
        void apply_evt_updateowner(apply_context& context);
        void apply_evt_transferevt(apply_context& context);

    ///@}  end action handlers

} } } /// namespace evt::contracts
