/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain {
class apply_context;
class version;
}}  // namespace evt::chain

namespace evt { namespace chain { namespace contracts {

struct abi_def;

template<typename T>
struct apply_action {
    static void invoke(apply_context&);
};

abi_def evt_contract_abi();
version evt_contract_abi_version();

}}}  // namespace evt::chain::contracts
