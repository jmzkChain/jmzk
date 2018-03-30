#include <algorithm>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
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

   if (!_write_scopes.empty()) {
      std::sort(_write_scopes.begin(), _write_scopes.end());
   }

   if (!_read_locks.empty()) {
      std::sort(_read_locks.begin(), _read_locks.end());
      // remove any write_scopes
      auto r_iter = _read_locks.begin();
      for( auto w_iter = _write_scopes.cbegin(); (w_iter != _write_scopes.cend()) && (r_iter != _read_locks.end()); ++w_iter) {
         shard_lock w_lock = {receiver, *w_iter};
         while(r_iter != _read_locks.end() && *r_iter < w_lock ) {
            ++r_iter;
         }

         if (*r_iter == w_lock) {
            r_iter = _read_locks.erase(r_iter);
         }
      }
   }

   // create a receipt for this
   vector<data_access_info> data_access;
   data_access.reserve(_write_scopes.size() + _read_locks.size());
   for (const auto& scope: _write_scopes) {
      auto key = boost::make_tuple(scope, receiver);
      const auto& scope_sequence = mutable_controller.get_database().find<scope_sequence_object, by_scope_receiver>(key);
      if (scope_sequence == nullptr) {
         try {
            mutable_controller.get_mutable_database().create<scope_sequence_object>([&](scope_sequence_object &ss) {
               ss.scope = scope;
               ss.receiver = receiver;
               ss.sequence = 1;
            });
         } FC_CAPTURE_AND_RETHROW((scope)(receiver));
         data_access.emplace_back(data_access_info{data_access_info::write, receiver, scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::write, receiver, scope, scope_sequence->sequence});
         try {
            mutable_controller.get_mutable_database().modify(*scope_sequence, [&](scope_sequence_object& ss) {
               ss.sequence += 1;
            });
         } FC_CAPTURE_AND_RETHROW((scope)(receiver));
      }
   }

   for (const auto& lock: _read_locks) {
      auto key = boost::make_tuple(lock.scope, lock.account);
      const auto& scope_sequence = mutable_controller.get_database().find<scope_sequence_object, by_scope_receiver>(key);
      if (scope_sequence == nullptr) {
         data_access.emplace_back(data_access_info{data_access_info::read, lock.account, lock.scope, 0});
      } else {
         data_access.emplace_back(data_access_info{data_access_info::read, lock.account, lock.scope, scope_sequence->sequence});
      }
   }

   results.applied_actions.emplace_back(action_trace {receiver, act, _pending_console_output.str(), 0, 0, move(data_access)});
   _pending_console_output = std::ostringstream();
   _read_locks.clear();
   _write_scopes.clear();
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

static bool scopes_contain(const vector<scope_name>& scopes, const scope_name& scope) {
   return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
}

static bool locks_contain(const vector<shard_lock>& locks, const account_name& account, const scope_name& scope) {
   return std::find(locks.begin(), locks.end(), shard_lock{account, scope}) != locks.end();
}

void apply_context::require_write_lock(const scope_name& scope) {
   if (trx_meta.allowed_write_locks) {
      EOS_ASSERT( locks_contain(**trx_meta.allowed_write_locks, receiver, scope), block_lock_exception, "write lock \"${a}::${s}\" required but not provided", ("a", receiver)("s",scope) );
   }

   if (!scopes_contain(_write_scopes, scope)) {
      _write_scopes.emplace_back(scope);
   }
}

void apply_context::require_read_lock(const account_name& account, const scope_name& scope) {
   if (trx_meta.allowed_read_locks || trx_meta.allowed_write_locks ) {
      bool locked_for_read = trx_meta.allowed_read_locks && locks_contain(**trx_meta.allowed_read_locks, account, scope);
      if (!locked_for_read && trx_meta.allowed_write_locks) {
         locked_for_read = locks_contain(**trx_meta.allowed_write_locks, account, scope);
      }
      EOS_ASSERT( locked_for_read , block_lock_exception, "read lock \"${a}::${s}\" required but not provided", ("a", account)("s",scope) );
   }

   if (!locks_contain(_read_locks, account, scope)) {
      _read_locks.emplace_back(shard_lock{account, scope});
   }
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

void apply_context::execute_deferred( deferred_transaction&& trx ) {
   try {
      FC_ASSERT( trx.expiration > (controller.head_block_time() + fc::milliseconds(2*config::block_interval_ms)),
                                   "transaction is expired when created" );

      FC_ASSERT( trx.execute_after < trx.expiration, "transaction expires before it can execute" );

      /// TODO: make default_max_gen_trx_count a producer parameter
      FC_ASSERT( results.generated_transactions.size() < config::default_max_gen_trx_count );

      FC_ASSERT( !trx.actions.empty(), "transaction must have at least one action");

      // todo: rethink this special case
      if (receiver != config::system_account_name) {
         controller.check_authorization(trx.actions, flat_set<public_key_type>(), false);
      }

      trx.sender = receiver; //  "Attempting to send from another account"
      trx.set_reference_block(controller.head_block_id());

      /// TODO: make sure there isn't already a deferred transaction with this ID or senderID?
      results.generated_transactions.emplace_back(move(trx));
   } FC_CAPTURE_AND_RETHROW((trx));
}

void apply_context::cancel_deferred( uint32_t sender_id ) {
   results.canceled_deferred.emplace_back(receiver, sender_id);
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

const char* to_string(contracts::table_key_type key_type) {
   switch(key_type) {
   case contracts::type_unassigned:
      return "unassigned";
   case contracts::type_i64:
      return "i64";
   case contracts::type_str:
      return "str";
   case contracts::type_i128i128:
      return "i128i128";
   case contracts::type_i64i64:
      return "i64i64";
   case contracts::type_i64i64i64:
      return "i64i64i64";
   default:
      return "<unkown table_key_type>";
   }
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
