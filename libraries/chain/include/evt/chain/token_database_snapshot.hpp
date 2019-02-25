/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/
#pragma once
#include <evt/chain/snapshot.hpp>

namespace evt { namespace chain {

class token_database;

namespace token_database_snapshot {

void add_to_snapshot(snapshot_writer_ptr snapshot, const token_database& db);
void read_from_snapshot(snapshot_reader_ptr snapshot, token_database& db);

}  // namespace token_database_snapshot

}}  // namespace evt::chain
