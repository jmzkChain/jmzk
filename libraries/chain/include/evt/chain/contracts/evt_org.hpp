/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain {
class token_database;
struct genesis_state;
}}  // namespace jmzk::chain

namespace jmzk { namespace chain { namespace contracts {

void initialize_jmzk_org(token_database& token_db, const genesis_state& genesis);
void update_jmzk_org(token_database& token_db, const genesis_state& genesis);

}}}  // namespace jmzk::chain::contracts
