/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <jmzk/chain/chain_config.hpp>
#include <jmzk/chain/exceptions.hpp>

namespace jmzk { namespace chain {

void 
chain_config::validate()const {
    jmzk_ASSERT(1 <= max_authority_depth, action_exception,
              "max authority depth should be at least 1");
}

}}  // namespace jmzk::chain