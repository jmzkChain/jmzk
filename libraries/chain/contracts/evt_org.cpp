/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_org.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/genesis_state.hpp>

namespace evt { namespace chain { namespace contracts {

void
initialize_evt_org(token_database& token_db, const genesis_state& genesis) {
    // Add reserved everiToken foundation group
    if(!token_db.exists_group(".everiToken")) {
        token_db.add_group(genesis.evt_org);
    }

    // Add reserved EVT fungible tokens
    if(!token_db.exists_fungible(evt_sym())) {
        token_db.add_fungible(genesis.evt);

        auto addr = address(N(.fungible), (name128)std::to_string(evt_sym().id()), 0);
        token_db.update_asset(addr, genesis.evt.total_supply);
    }

    // Add reserved Pined EVT fungible tokens
    if(!token_db.exists_fungible(pevt_sym())) {
        token_db.add_fungible(genesis.pevt);
    }
}

}}}  // namespace evt::chain::contracts
