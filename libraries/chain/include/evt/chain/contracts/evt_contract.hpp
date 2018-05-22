/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/version.hpp>

namespace evt { namespace chain {
class apply_context;
}}  // namespace evt::chain

namespace evt { namespace chain { namespace contracts {

struct abi_def;

/**
* @defgroup native_action_handlers Native Action Handlers
*/
///@{

void apply_evt_newdomain(apply_context& context);
void apply_evt_issuetoken(apply_context& context);
void apply_evt_transfer(apply_context& context);
void apply_evt_newgroup(apply_context& context);
void apply_evt_updategroup(apply_context& context);
void apply_evt_updatedomain(apply_context& context);

void apply_evt_newaccount(apply_context& context);
void apply_evt_updateowner(apply_context& context);
void apply_evt_transferevt(apply_context& context);

abi_def evt_contract_abi();
version evt_contract_abi_version();

///@}  end action handlers

}}}  // namespace evt::chain::contracts
