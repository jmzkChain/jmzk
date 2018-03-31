/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/block.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/tokendb.hpp>
#include <fc/utility.hpp>
#include <sstream>
#include <algorithm>

namespace chainbase { class database; }

namespace eosio { namespace chain {

class chain_controller;

class apply_context {
public:
      apply_context(chain_controller& con, chainbase::database& db, evt::chain::tokendb& tokendb, const action& a, const transaction_metadata& trx_meta, uint32_t depth=0)

      :controller(con),
       db(db),
       act(a),
       mutable_controller(con),
       mutable_db(db),
       mutable_tokendb(tokendb),
       trx_meta(trx_meta),
       recurse_depth(depth)
       {}

      void exec();

      void execute_inline( action &&a );
      void execute_deferred( deferred_transaction &&trx );
      void cancel_deferred( uint32_t sender_id );
      
      void require_write_lock(const scope_name& scope);
      void require_read_lock(const account_name& account, const scope_name& scope);

    /**
       * @return true if account exists, false if it does not
       */
      bool is_account(const account_name& account)const;

      /**
       * Requires that the current action be delivered to account
       */
      void require_recipient(account_name account);

      /**
       * Return true if the current action has already been scheduled to be
       * delivered to the specified account.
       */
      bool has_recipient(account_name account)const;

      vector<account_name> get_active_producers() const;

      const bytes&         get_packed_transaction();

      const chain_controller&       controller;
      const chainbase::database&    db;  ///< database where state is stored
      const action&                 act; ///< message being applied
      account_name                  receiver; ///< the code that is currently running
      bool                          privileged   = false;

      chain_controller&             mutable_controller;
      chainbase::database&          mutable_db;
      evt::chain::tokendb           mutable_tokendb;


      const transaction_metadata&   trx_meta;

      struct apply_results {
         vector<action_trace>          applied_actions;
         vector<deferred_transaction>  generated_transactions;
         vector<deferred_reference>    canceled_deferred;
      };

      apply_results results;

      template<typename T>
      void console_append(T val) {
         _pending_console_output << val;
      }

      template<typename T, typename ...Ts>
      void console_append(T val, Ts ...rest) {
         console_append(val);
         console_append(rest...);
      };

      inline void console_append_formatted(const string& fmt, const variant_object& vo) {
         console_append(fc::format_string(fmt, vo));
      }

      void checktime(uint32_t instruction_count) const;

      int get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const;

      void update_db_usage( const account_name& payer, int64_t delta );

      uint32_t                                    recurse_depth;  // how deep inline actions can recurse

      static constexpr uint32_t base_row_fee = 200;

   private:
      void append_results(apply_results &&other) {
         fc::move_append(results.applied_actions, move(other.applied_actions));
         fc::move_append(results.generated_transactions, move(other.generated_transactions));
         fc::move_append(results.canceled_deferred, move(other.canceled_deferred));
      }

      void exec_one();
      
      vector<account_name>                _notified; ///< keeps track of new accounts to be notifed of current message
      vector<action>                      _inline_actions; ///< queued inline messages
      std::ostringstream                  _pending_console_output;

      vector<shard_lock>                  _read_locks;
      vector<scope_name>                  _write_scopes;
      bytes                               _cached_trx;
};

using apply_handler = std::function<void(apply_context&)>;

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::apply_context::apply_results, (applied_actions)(generated_transactions))
