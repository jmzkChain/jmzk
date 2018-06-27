/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/chain/chain_config.hpp>
#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

void 
chain_config::validate()const {
    EVT_ASSERT(1 <= max_authority_depth, action_exception,
              "max authority depth should be at least 1");
}

}}  // namespace evt::chain