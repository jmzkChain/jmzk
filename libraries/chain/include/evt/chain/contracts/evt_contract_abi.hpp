/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain {
class version;
}}  // namespace evt::chain

namespace evt { namespace chain { namespace contracts {

struct abi_def;

abi_def evt_contract_abi();
version evt_contract_abi_version();

}}}  // namespace evt::chain::contracts
