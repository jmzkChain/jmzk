/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/lua_engine.hpp>
#include <lua.hpp>

namespace evt { namespace chain { namespace contracts {

namespace internal {

thread_local void* G_PKEY;

}  // namespace internal

void
lua_engine::invoke_filter(const token_database& tokendb, const action& act) {
    

}

}}}  // namespac evt::chain::contracts
