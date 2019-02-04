/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/chain/execution_context.hpp>

namespace evt { namespace chain {

void
execution_context::set_versions(const std::vector<action_ver>& acts) {
    for(auto& act : acts) {
        if(act.ver > get_current_version(act.act)) {
            set_version(act.act, act.ver);
        }
    }
}

}}  // namespace evt::chain
