#include <algorithm>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace eosio { namespace chain {
void apply_context::exec_one()
{
   try {
      const auto &a = mutable_controller.get_database().get<account_object, by_name>(receiver);
      privileged = a.privileged;

      auto native = mutable_controller.find_apply_handler(receiver, act.account, act.name);
      if (native) {
        (*native)(*this);
      } else {
        // Removed WASM executing path
        assert(false);
      }
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

   results.applied_actions.emplace_back(action_trace {receiver, act, _pending_console_output.str(), 0, 0 });
   _pending_console_output = std::ostringstream();
}

void apply_context::exec()
{
   _notified.push_back(act.account);
   for( uint32_t i = 0; i < _notified.size(); ++i ) {
      receiver = _notified[i];
      exec_one();
   }

   for( uint32_t i = 0; i < _inline_actions.size(); ++i ) {
      EOS_ASSERT( recurse_depth < config::max_recursion_depth, transaction_exception, "inline action recursion depth reached" );
      apply_context ncontext( mutable_controller, mutable_db, mutable_tokendb, _inline_actions[i], trx_meta, recurse_depth + 1 );
      ncontext.exec();
      append_results(move(ncontext.results));
   }

} /// exec()

bool apply_context::is_account( const account_name& account )const {
   return nullptr != db.find<account_object,by_name>( account );
}

bool apply_context::has_recipient( account_name code )const {
   for( auto a : _notified )
      if( a == code )
         return true;
   return false;
}

void apply_context::require_recipient( account_name code ) {
   if( !has_recipient(code) )
      _notified.push_back(code);
}


/**
 *  This will execute an action after checking the authorization. Inline transactions are 
 *  implicitly authorized by the current receiver (running code). This method has significant
 *  security considerations and several options have been considered:
 *
 *  1. priviledged accounts (those marked as such by block producers) can authorize any action
 *  2. all other actions are only authorized by 'receiver' which means the following:
 *         a. the user must set permissions on their account to allow the 'receiver' to act on their behalf
 *  
 *  Discarded Implemenation:  at one point we allowed any account that authorized the current transaction
 *   to implicitly authorize an inline transaction. This approach would allow privelege escalation and
 *   make it unsafe for users to interact with certain contracts.  We opted instead to have applications
 *   ask the user for permission to take certain actions rather than making it implicit. This way users
 *   can better understand the security risk.
 */
// TODO: (EVT) Need to rethink functions below for our cases
void apply_context::execute_inline( action&& a ) {
   if ( !privileged ) { 
      controller.check_authorization({a}, flat_set<public_key_type>(), false); 
   }
   _inline_actions.emplace_back( move(a) );
}

vector<account_name> apply_context::get_active_producers() const {
   const auto& gpo = controller.get_global_properties();
   vector<account_name> accounts;
   for(const auto& producer : gpo.active_producers.producers)
      accounts.push_back(producer.producer_name);

   return accounts;
}

void apply_context::checktime(uint32_t instruction_count) const {
   if (trx_meta.processing_deadline && fc::time_point::now() > (*trx_meta.processing_deadline)) {
      throw checktime_exceeded();
   }
}


const bytes& apply_context::get_packed_transaction() {
   if( !trx_meta.packed_trx.size() ) {
      if (_cached_trx.empty()) {
         auto size = fc::raw::pack_size(trx_meta.trx());
         _cached_trx.resize(size);
         fc::datastream<char *> ds(_cached_trx.data(), size);
         fc::raw::pack(ds, trx_meta.trx());
      }

      return _cached_trx;
   }

   return trx_meta.packed_trx;
}

int apply_context::get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const
{
   const transaction& trx = trx_meta.trx();
   const action* act = nullptr;
   if( type == 1 ) {
      if( index >= trx.actions.size() )
         return -1;
      act = &trx.actions[index];
   }

   auto ps = fc::raw::pack_size( *act );
   if( ps <= buffer_size ) {
      fc::datastream<char*> ds(buffer, buffer_size);
      fc::raw::pack( ds, *act );
   }
   return ps;
}

} } /// eosio::chain
