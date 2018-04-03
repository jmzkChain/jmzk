/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <boost/container/flat_set.hpp>

namespace fc { class variant; }

namespace eosio {
   using chain::chain_controller;
   using std::unique_ptr;
   using namespace appbase;
   using chain::name;
   using chain::uint128_t;
   using chain::public_key_type;
   using fc::optional;
   using boost::container::flat_set;
   using chain::asset;
   using chain::authority;
   using chain::account_name;
   using chain::contracts::abi_def;
   using chain::contracts::abi_serializer;

namespace chain_apis {
struct empty{};

struct permission {
   name              perm_name;
   name              parent;
   authority         required_auth;
};

template<typename>
struct resolver_factory;

class read_only {
   const chain_controller& db;

public:
   static const string KEYi64;
   static const string KEYstr;
   static const string KEYi128i128;
   static const string KEYi64i64i64;
   static const string PRIMARY;
   static const string SECONDARY;
   static const string TERTIARY;
   
   read_only(const chain_controller& db)
      : db(db) {}

   using get_info_params = empty;

   struct get_info_results {
      string                  server_version;
      uint32_t                head_block_num = 0;
      uint32_t                last_irreversible_block_num = 0;
      chain::block_id_type    head_block_id;
      fc::time_point_sec      head_block_time;
      account_name            head_block_producer;
      string                  recent_slots;
      double                  participation_rate = 0;
   };
   get_info_results get_info(const get_info_params&) const;

   struct producer_info {
      name                       producer_name;
   };


   struct get_account_results {
      name                       account_name;
      vector<permission>         permissions;
   };

   struct get_account_params {
      name account_name;
   };
   get_account_results get_account( const get_account_params& params )const;


   struct abi_json_to_bin_params {
      name         code;
      name         action;
      fc::variant  args;
   };
   struct abi_json_to_bin_result {
      vector<char>   binargs;
      vector<name>   required_scope;
      vector<name>   required_auth;
   };
      
   abi_json_to_bin_result abi_json_to_bin( const abi_json_to_bin_params& params )const;


   struct abi_bin_to_json_params {
      name         code;
      name         action;
      vector<char> binargs;
   };
   struct abi_bin_to_json_result {
      fc::variant    args;
      vector<name>   required_scope;
      vector<name>   required_auth;
   };
      
   abi_bin_to_json_result abi_bin_to_json( const abi_bin_to_json_params& params )const;


   struct get_required_keys_params {
      fc::variant transaction;
      flat_set<public_key_type> available_keys;
   };
   struct get_required_keys_result {
      flat_set<public_key_type> required_keys;
   };

   get_required_keys_result get_required_keys( const get_required_keys_params& params)const;


   struct get_block_params {
      string block_num_or_id;
   };

   fc::variant get_block(const get_block_params& params) const;

   struct get_currency_balance_params {
      name             code;
      name             account;
      optional<string> symbol;
   };

   vector<asset> get_currency_balance( const get_currency_balance_params& params )const;

   struct get_currency_stats_params {
      name             code;
      optional<string> symbol;
   };

   struct get_currency_stats_result {
      asset        supply;
   };

   fc::variant get_currency_stats( const get_currency_stats_params& params )const;

   friend struct resolver_factory<read_only>;
};

class read_write {
   chain_controller& db;
   uint32_t skip_flags;
public:
   read_write(chain_controller& db, uint32_t skip_flags) : db(db), skip_flags(skip_flags) {}

   using push_block_params = chain::signed_block;
   using push_block_results = empty;
   push_block_results push_block(const push_block_params& params);

   using push_transaction_params = fc::variant_object;
   struct push_transaction_results {
      chain::transaction_id_type  transaction_id;
      fc::variant                 processed;
   };
   push_transaction_results push_transaction(const push_transaction_params& params);


   using push_transactions_params  = vector<push_transaction_params>;
   using push_transactions_results = vector<push_transaction_results>;
   push_transactions_results push_transactions(const push_transactions_params& params);

   friend resolver_factory<read_write>;
};
} // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   chain_apis::read_only get_read_only_api() const { return chain_apis::read_only(chain()); }
   chain_apis::read_write get_read_write_api();

   bool accept_block(const chain::signed_block& block, bool currently_syncing);
   void accept_transaction(const chain::packed_transaction& trx);

   bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

   // return true if --skip-transaction-signatures passed to eosd
   bool is_skipping_transaction_signatures() const;

   // Only call this in plugin_initialize() to modify chain_controller constructor configuration
   chain_controller::controller_config& chain_config();
   // Only call this after plugin_startup()!
   chain_controller& chain();
   // Only call this after plugin_startup()!
   const chain_controller& chain() const;

  void get_chain_id (chain::chain_id_type &cid) const;

private:
   unique_ptr<class chain_plugin_impl> my;
};

}

FC_REFLECT( eosio::chain_apis::permission, (perm_name)(parent)(required_auth) )
FC_REFLECT(eosio::chain_apis::empty, )
FC_REFLECT(eosio::chain_apis::read_only::get_info_results,
  (server_version)(head_block_num)(last_irreversible_block_num)(head_block_id)(head_block_time)(head_block_producer)
  (recent_slots)(participation_rate))
FC_REFLECT(eosio::chain_apis::read_only::get_block_params, (block_num_or_id))
  
FC_REFLECT( eosio::chain_apis::read_write::push_transaction_results, (transaction_id)(processed) )
  
FC_REFLECT( eosio::chain_apis::read_only::get_currency_balance_params, (code)(account)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_params, (code)(symbol));
FC_REFLECT( eosio::chain_apis::read_only::get_currency_stats_result, (supply));

FC_REFLECT( eosio::chain_apis::read_only::get_account_results, (account_name)(permissions) )
FC_REFLECT( eosio::chain_apis::read_only::get_account_params, (account_name) )
FC_REFLECT( eosio::chain_apis::read_only::producer_info, (producer_name) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_params, (code)(action)(args) )
FC_REFLECT( eosio::chain_apis::read_only::abi_json_to_bin_result, (binargs)(required_scope)(required_auth) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_params, (code)(action)(binargs) )
FC_REFLECT( eosio::chain_apis::read_only::abi_bin_to_json_result, (args)(required_scope)(required_auth) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_params, (transaction)(available_keys) )
FC_REFLECT( eosio::chain_apis::read_only::get_required_keys_result, (required_keys) )
