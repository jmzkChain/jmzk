/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

template<uint64_t>
struct apply_action {};

}}}  // namespace jmzk::chain::contracts

#include <jmzk/chain/contracts/jmzk_contract_common.hpp>
#include <jmzk/chain/contracts/jmzk_contract_metas.hpp>
#include <jmzk/chain/contracts/jmzk_contract_nft.hpp>
#include <jmzk/chain/contracts/jmzk_contract_ft.hpp>
#include <jmzk/chain/contracts/jmzk_contract_group.hpp>
#include <jmzk/chain/contracts/jmzk_contract_lock.hpp>
#include <jmzk/chain/contracts/jmzk_contract_suspend.hpp>
#include <jmzk/chain/contracts/jmzk_contract_bonus.hpp>
#include <jmzk/chain/contracts/jmzk_contract_jmzklink.hpp>
#include <jmzk/chain/contracts/jmzk_contract_utils.hpp>
#include <jmzk/chain/contracts/jmzk_contract_staking.hpp>
