/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain {
class version;
}}  // namespace jmzk::chain

namespace jmzk { namespace chain { namespace contracts {

struct abi_def;

abi_def jmzk_contract_abi();
version jmzk_contract_abi_version();

}}}  // namespace jmzk::chain::contracts
