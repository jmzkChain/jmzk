/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/chain/contracts/genesis_state.hpp>

// these are required to serialize a genesis_state
#include <fc/smart_ref_impl.hpp>   // required for gcc in release mode

namespace evt { namespace chain { namespace contracts {


chain::chain_id_type genesis_state::compute_chain_id() const {
   return initial_chain_id;
}

} } } // namespace evt::chain::contracts
