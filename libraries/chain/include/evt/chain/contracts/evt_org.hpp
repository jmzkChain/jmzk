/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain {
class token_database;
struct genesis_state;
}}  // namespace evt::chain

namespace evt { namespace chain { namespace contracts {

void initialize_evt_org(token_database& token_db, const genesis_state& genesis);
void update_evt_org(token_database& token_db, const genesis_state& genesis);

}}}  // namespace evt::chain::contracts
