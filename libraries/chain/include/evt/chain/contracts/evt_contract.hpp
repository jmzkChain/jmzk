/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

template<uint64_t>
struct apply_action {};

}}}  // namespace evt::chain::contracts

#include <evt/chain/contracts/evt_contract_common.hpp>
#include <evt/chain/contracts/evt_contract_metas.hpp>
#include <evt/chain/contracts/evt_contract_nft.hpp>
#include <evt/chain/contracts/evt_contract_ft.hpp>
#include <evt/chain/contracts/evt_contract_group.hpp>
#include <evt/chain/contracts/evt_contract_lock.hpp>
#include <evt/chain/contracts/evt_contract_suspend.hpp>
#include <evt/chain/contracts/evt_contract_bonus.hpp>
#include <evt/chain/contracts/evt_contract_evtlink.hpp>
#include <evt/chain/contracts/evt_contract_utils.hpp>
#include <evt/chain/contracts/evt_contract_staking.hpp>
