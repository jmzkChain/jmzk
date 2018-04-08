/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @defgroup eosclienttool EOS Command Line Client Reference
 *  @brief Tool for sending transactions and querying state from @ref eosd
 *  @ingroup eosclienttool
 */

/**
  @defgroup eosclienttool

  @section intro Introduction to EOSC

  `eosc` is a command line tool that interfaces with the REST api exposed by @ref eosd. In order to use `eosc` you will need to
  have a local copy of `eosd` running and configured to load the 'eosio::chain_api_plugin'.

   eosc contains documentation for all of its commands. For a list of all commands known to eosc, simply run it with no arguments:
```
$ ./eosc
Command Line Interface to Eos Daemon
Usage: ./eosc [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help actions and exit
  -H,--host TEXT=localhost    the host where eosd is running
  -p,--port UINT=8888         the port where eosd is running
  --wallet-host TEXT=localhost
                              the host where eos-walletd is running
  --wallet-port UINT=8888     the port where eos-walletd is running

Subcommands:
  create                      Create various items, on and off the blockchain
  get                         Retrieve various items and information from the blockchain
  set                         Set or update blockchain state
  transfer                    Transfer EOS from account to account
  wallet                      Interact with local wallet
  push                        Push arbitrary transactions to the blockchain

```
To get help with any particular subcommand, run it with no arguments as well:
```
$ ./eosc create
Create various items, on and off the blockchain
Usage: ./eosc create SUBCOMMAND

Subcommands:
  key                         Create a new keypair and print the public and private keys
  account                     Create a new account on the blockchain

$ ./eosc create account
Create a new account on the blockchain
Usage: ./eosc create account [OPTIONS] creator name OwnerKey ActiveKey

Positionals:
  creator TEXT                The name of the account creating the new account
  name TEXT                   The name of the new account
  OwnerKey TEXT               The owner public key for the new account
  ActiveKey TEXT              The active public key for the new account

Options:
  -s,--skip-signature         Specify that unlocked wallet keys should not be used to sign transaction
```
*/
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/io/fstream.hpp>
#include <fc/exception/exception.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/string/classification.hpp>


#include "CLI11.hpp"
#include "help_text.hpp"
#include "localize.hpp"
#include "config.hpp"
#include "httpc.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::utilities;
using namespace eosio::client::help;
using namespace eosio::client::http;
using namespace eosio::client::localize;
using namespace eosio::client::config;
using namespace boost::filesystem;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );
FC_DECLARE_EXCEPTION( localized_exception, 10000000, "an error occured" );
#define EOSC_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                   \
        std::cerr << localized( __VA_ARGS__ ) << std::endl;  \
        FC_THROW_EXCEPTION( explained_exception, #TEST ); \
      }                                                   \
    FC_MULTILINE_MACRO_END \
  )

string program = "eosc";
string host = "localhost";
uint32_t port = 8888;

// restricting use of wallet to localhost
string wallet_host = "localhost";
uint32_t wallet_port = 8888;

auto   tx_expiration = fc::seconds(30);
bool   tx_force_unique = false;
bool   tx_dont_broadcast = false;
bool   tx_skip_sign = false;
vector<string> tx_permission;

void add_standard_transaction_options(CLI::App* cmd, string default_permission = "") {
   CLI::callback_t parse_exipration = [](CLI::results_t res) -> bool {
      double value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }
      
      tx_expiration = fc::seconds(static_cast<uint64_t>(value_s));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_exipration, localized("set the time in seconds before a transaction expires, defaults to 30s"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
   cmd->add_flag("-s,--skip-sign", tx_skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   cmd->add_flag("-d,--dont-broadcast", tx_dont_broadcast, localized("don't broadcast transaction to the network (just print to stdout)"));

   string msg = "An account and permission level to authorize, as in 'account@permission'";
   if(!default_permission.empty())
      msg += " (defaults to '" + default_permission + "')";
   cmd->add_option("-p,--permission", tx_permission, localized(msg.c_str()));
}

string generate_nonce_value() {
   return fc::to_string(fc::time_point::now().time_since_epoch().count());
}

chain::action generate_nonce() {
   auto v = generate_nonce_value();
   variant nonce = fc::mutable_variant_object()
         ("value", v);
   return chain::action( {}, config::system_account_name, "nonce", fc::raw::pack(nonce));
}

vector<chain::permission_level> get_account_permissions(const vector<string>& permissions) {
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
      vector<string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      EOSC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
      return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
   });
   vector<chain::permission_level> accountPermissions;
   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
   return accountPermissions;
}

template<typename T>
fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const T& v ) { return eosio::client::http::call( server, port, path, fc::variant(v) ); }

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return eosio::client::http::call( host, port, path, fc::variant(v) ); }

eosio::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<eosio::chain_apis::read_only::get_info_results>();
}

void sign_transaction(signed_transaction& trx) {
   // TODO better error checking
   const auto& public_keys = call(wallet_host, wallet_port, wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
         ("transaction", (transaction)trx)
         ("available_keys", public_keys);
   const auto& required_keys = call(host, port, get_required_keys, get_arg);
   // TODO determine chain id
   fc::variants sign_args = {fc::variant(trx), required_keys["required_keys"], fc::variant(chain_id_type{})};
   const auto& signed_trx = call(wallet_host, wallet_port, wallet_sign_trx, sign_args);
   trx = signed_trx.as<signed_transaction>();
}

fc::variant push_transaction( signed_transaction& trx, packed_transaction::compression_type compression = packed_transaction::none ) {
   auto info = get_info();
   trx.expiration = info.head_block_time + tx_expiration;
   trx.set_reference_block(info.head_block_id);

   if (tx_force_unique) {
      trx.context_free_actions.emplace_back( generate_nonce() );
   }

   if (!tx_skip_sign) {
      sign_transaction(trx);
   }

   if (!tx_dont_broadcast) {
      return call(push_txn_func, packed_transaction(trx, compression));
   } else {
      return fc::variant(trx);
   }
}

fc::variant push_actions(std::vector<chain::action>&& actions, packed_transaction::compression_type compression = packed_transaction::none ) {
   signed_transaction trx;
   trx.actions = std::forward<decltype(actions)>(actions);

   return push_transaction(trx, compression);
}

void send_actions(std::vector<chain::action>&& actions, packed_transaction::compression_type compression = packed_transaction::none ) {
   std::cout << fc::json::to_pretty_string(push_actions(std::forward<decltype(actions)>(actions), compression)) << std::endl;
}

void send_transaction( signed_transaction& trx, packed_transaction::compression_type compression = packed_transaction::none  ) {
   std::cout << fc::json::to_pretty_string(push_transaction(trx, compression)) << std::endl;
}

template<typename T>
chain::action create_action(const domain_name& domain, const domain_key& key, const T& value) {
   return action(domain, key, value);
}

auto parse_permission = [](auto& jsonOrFile) {
  try {
    fc::variant parsedPermission;
    if (boost::istarts_with(jsonOrFile, "{")) {
        parsedPermission = fc::json::from_string(jsonOrFile);
    } else {
        parsedPermission = fc::json::from_file(jsonOrFile);
    }
    auto permission = parsedPermission.as<permission_def>();
    return permission;
  } EOS_CAPTURE_AND_RETHROW(permission_type_exception, "Fail to parse Permission JSON")
};

auto parse_groups = [](auto& jsonOrFile) {
  try {
    fc::variant parsedGroups;
    if (boost::istarts_with(jsonOrFile, "{")) {
        parsedGroups = fc::json::from_string(jsonOrFile);
    } else {
        parsedGroups = fc::json::from_file(jsonOrFile);
    }
    auto groups = parsedGroups.as<std::vector<group_def>>();
    return groups;
  } EOS_CAPTURE_AND_RETHROW(groups_type_exception, "Fail to parse Groups JSON")
};

struct set_domain_subcommands {
   string name;
   string issuer;
   string issue;
   string transfer;
   string manage;
   string groups;

   set_domain_subcommands(CLI::App* actionRoot) {
      auto newdomain = actionRoot->add_subcommand("new", localized("Add new domain"), false);
      newdomain->add_option("name", name, localized("The name of new domain"))->required();
      newdomain->add_option("issuer", issuer, localized("The public key of the issuer"))->required();
      newdomain->add_option("issue", issue, localized("JSON string or filename defining ISSUE permission"))->required();
      newdomain->add_option("transfer", transfer, localized("JSON string or filename defining TRANSFER permission"))->required();
      newdomain->add_option("manage", manage, localized("JSON string or filename defining MANAGE permission"))->required();
      newdomain->add_option("groups", groups, localized("JSON string or filename defining groups which are new defined"))->required();

      add_standard_transaction_options(newdomain);

      newdomain->set_callback([this, &] {
         newdomain nd;
         nd.name = name128(name);
         nd.issuer = public_key(issuer);
         nd.issue = parse_permission(issue);
         nd.transfer = parse_permission(transfer);
         nd.manage = parse_permission(manage);
         nd.groups = parse_groups(groups);
         
         auto act = crate_action("domain", (domain_key)nd.name, nd);
         send_actions({ act });
      });
      
      auto updatedomain = actionRoot->add_subcommand("update", localized("Update existing domain"), false);
      updatedomain->add_option("name", name, localized("The name of new domain"))->required();
      updatedomain->add_option("issue", issue, localized("JSON string or filename defining ISSUE permission"))->required();
      updatedomain->add_option("transfer", transfer, localized("JSON string or filename defining TRANSFER permission")->required();
      updatedomain->add_option("manage", manage, localized("JSON string or filename defining MANAGE permission"))->required();
      updatedomain->add_option("groups", groups, localized("JSON string or filename defining groups which are new defined"))->required();

      add_standard_transaction_options(updatedomain);

      updatedomain->set_callback([this, &] {
         updatedomain ud;
         ud.name = name128(name);
         ud.issue = parse_permission(issue);
         ud.transfer = parse_permission(transfer);
         ud.manage = parse_permission(manage);
         ud.groups = parse_groups(groups);
         
         auto act = create_action("domain", (domain_key)ud.name, ud);
         send_actions({ act });
      });
   }
};

struct set_issue_token_subcommand {
   string domain;
   std::vector<string> names;
   std::vector<string> owner;

   set_issue_token_subcommand(CLI::App* actionRoot) {
      auto issue = actionRoot->add_subcommand("issue", localized("Issue new tokens in specific domain"), false);
      issue->add_option("domain", domain, localized("Name of the domain where token issued"))->required();
      issue->add_option("names", names, localized("Names of tokens will be issued"))->required();
      issue->add_option("owner", owner, localized("Owner that issued tokens belongs to"))->required();

      add_standard_transaction_options(issue);

      issue->set_callback([this] {
         issuetoken it;
         it.domain = name128(domain);
         it.names = names;
         it.owner = owner;

         auto act = create_action(it.domain, N128(issue), it);
         send_actions({ act });
      });
   }
};

struct set_transfer_token_subcommand {
   string domain;
   string name;
   std::vector<string> to;

   set_transfer_token_subcommand(CLI::App* actionRoot) {
      auto transfer = actionRoot->add_subcommand("transfer", localized("Transfer token"), false);
      transfer->add_option("domain", domain, localized("Name of the domain where token existed"))->required();
      transfer->add_option("name", name, localized("Name of the token to be transfered"))->required();
      transfer->add_option("to", to, localized("User list receives this token"))->required();
      
      add_standard_transaction_options(transfer);

      transfer->set_callback([this] {
         transfer tt;
         tt.domain = name128(domain);
         tt.name = name128(name);
         tt.to = to;

         auto act = create_action(it.domain, (domain_key)tt.name, tt);
         send_actions({ act });
      });
   }
};

struct set_group_subcommands {
   string id;
   string key;
   uint32_t threshold;
   string keys;

   
   set_group_subcommands(CLI::App* actionRoot) {
      auto updategroup = actionRoot->add_subcommand("update", localized("Update specific permission group, id or key must provide at least one."), false);
      updategroup->add_option("id", id, localized("Id of the permission group to be updated"));
      updategroup->add_option("key", key, localized("Key of permission group to be updated"));
      updategroup->add_option("threshold", threshold, localized("Threshold of permission group"))->required();
      updategroup->add_option("keys", keys, localized("JSON string or filename defining the keys of permission group"))->required();

      add_standard_transaction_options(updategroup);

      updategroup->set_callback([this] {
         updategroup ug;
         FC_ASSERT(id.empty() && key.empty(), "Must provide either id or key");
         if(!id.empty()) {
             ug.id = group_id::from_base58(id);
         }
         if(!key.empty()) {
             ug.id = group_id::from_group_key(public_key(key));
         }
         ug.threshold = threshold;

         fc::variant parsedKeys;
         if (boost::istarts_with(keys, "{")) {
             parsedKeys = fc::json::from_string(keys);
         } else {
             parsedKeys = fc::json::from_file(keys);
         }
         ug.keys = parsedKeys.as<std::vector<key_weight>>();

         auto act = create_action("group", (domain_key)ug.id, ug);
         send_actions({ act });
      });
   }
};

struct set_get_domain_subcommand {
   string name;

   set_get_domain_subcommand(CLI::App* actionRoot) {
       auto get_domain = actionRoot->add_subcommand("domain", localized("Retrieve a domain information"), false);
       get_domain->add_option("name", name, localized("Name of domain to be retrieved"))->required();

       get_domain->set_callback([this] {
          auto arg = fc::mutable_variant_object("name", name);
          std::cout << fc::json::to_pretty_string(call(get_domain_func, arg)) << std::endl;
       });
   }
};

struct set_get_token_subcommand {
   string domain;
   string name;

   set_get_domain_subcommand(CLI::App* actionRoot) {
       auto get_token = actionRoot->add_subcommand("token", localized("Retrieve a token information"), false);
       get_token->add_option("domain", name, localized("Domain name of token to be retrieved"))->required();
       get_token->add_option("name", name, localized("Name of token to be retrieved"))->required();

       get_token->set_callback([this] {
          auto arg = fc::mutable_variant_object("domain", domain)("name", name);
          std::cout << fc::json::to_pretty_string(call(get_token_func, arg)) << std::endl;
       });
   }
};

struct set_get_group_subcommand {
   string id;
   string key;

   set_get_group_subcommand(CLI::App* actionRoot) {
       auto get_group = actionRoot->add_subcommand("group", localized("Retrieve a permission group information"), false);
       get_group->add_option("id", id, localized("Id of group to be retrieved"));
       get_group->add_option("key", key, localized("Key of group to be retrieved"));

       get_group->set_callback([this] {
          group_id gid;
          FC_ASSERT(id.empty() && key.empty(), "Must provide either id or key");
          if(!id.empty()) {
             gid = group_id::from_base58(id);
          }
          if(!key.empty()) {
             gid = group_id::from_group_key(public_key(key));
          }

          auto arg = fc::mutable_variant_object("id", gid.to_base58());
          std::cout << fc::json::to_pretty_string(call(get_group_func, arg)) << std::endl;
       });
   }
};

int main( int argc, char** argv ) {
   fc::path binPath = argv[0];
   if (binPath.is_relative()) {
      binPath = relative(binPath, current_path()); 
   }

   setlocale(LC_ALL, "");
   bindtextdomain(locale_domain, locale_path);
   textdomain(locale_domain);

   CLI::App app{"Command Line Interface to Eos Client"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localized("the host where eosd is running"), true );
   app.add_option( "-p,--port", port, localized("the port where eosd is running"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, localized("output verbose actions on error"));

   auto version = app.add_subcommand("version", localized("Retrieve version information"), false);
   version->require_subcommand();

   version->add_subcommand("client", localized("Retrieve version information of the client"))->set_callback([] {
     std::cout << localized("Build version: ${ver}", ("ver", eosio::client::config::version_str)) << std::endl;
   });

   // Create subcommand
   auto create = app.add_subcommand("create", localized("Create various items, on and off the blockchain"), false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", localized("Create a new keypair and print the public and private keys"))->set_callback( [](){
      auto pk    = private_key_type::generate();
      auto privs = string(pk);
      auto pubs  = string(pk.get_public_key());
      std::cout << localized("Private key: ${key}", ("key",  privs) ) << std::endl;
      std::cout << localized("Public key: ${key}", ("key", pubs ) ) << std::endl;
   });

   // Get subcommand
   auto get = app.add_subcommand("get", localized("Retrieve various items and information from the blockchain"), false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localized("Get current blockchain information"))->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", localized("Retrieve a full block from the blockchain"), false);
   getBlock->add_option("block", blockArg, localized("The number or ID of the block to retrieve"))->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"), false);
   getTransaction->add_option("id", transactionId, localized("ID of the transaction to retrieve"))->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"), false);
   getTransactions->add_option("account_name", account_name, localized("name of account to query on"))->required();
   getTransactions->add_option("skip_seq", skip_seq, localized("Number of most recent transactions to skip (0 would start at most recent transaction)"));
   getTransactions->add_option("num_seq", num_seq, localized("Number of transactions to return"));
   getTransactions->set_callback([&] {
      auto arg = (skip_seq.empty())
                  ? fc::mutable_variant_object( "account_name", account_name)
                  : (num_seq.empty())
                     ? fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)
                     : fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)("num_seq", num_seq);
      auto result = call(get_transactions_func, arg);
      std::cout << fc::json::to_pretty_string(call(get_transactions_func, arg)) << std::endl;


      const auto& trxs = result.get_object()["transactions"].get_array();
      for( const auto& t : trxs ) {
         const auto& tobj = t.get_object();
         int64_t seq_num  = tobj["seq_num"].as<int64_t>();
         string  id       = tobj["transaction_id"].as_string();
         const auto& trx  = tobj["transaction"].get_object();
         const auto& data = trx["data"].get_object();
         const auto& exp  = data["expiration"].as<fc::time_point_sec>();
         const auto& msgs = data["actions"].get_array();
         std::cout << tobj["seq_num"].as_string() <<"] " << id << "  " << data["expiration"].as_string() << std::endl;
      }

   });

   set_get_domain_subcommand get_domain(get);
   set_get_token_subcommand get_token(get);
   set_get_group_subcommand get_group(get);

   // Net subcommand 
   string new_host;
   auto net = app.add_subcommand( "net", localized("Interact with local p2p network connections"), false );
   net->require_subcommand();
   auto connect = net->add_subcommand("connect", localized("start a new connection to a peer"), false);
   connect->add_option("host", new_host, localized("The hostname:port to connect to."))->required();
   connect->set_callback([&] {
      const auto& v = call(host, port, net_connect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto disconnect = net->add_subcommand("disconnect", localized("close an existing connection"), false);
   disconnect->add_option("host", new_host, localized("The hostname:port to disconnect from."))->required();
   disconnect->set_callback([&] {
      const auto& v = call(host, port, net_disconnect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto status = net->add_subcommand("status", localized("status of existing connection"), false);
   status->add_option("host", new_host, localized("The hostname:port to query status of connection"))->required();
   status->set_callback([&] {
      const auto& v = call(host, port, net_status, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto connections = net->add_subcommand("peers", localized("status of all existing peers"), false);
   connections->set_callback([&] {
      const auto& v = call(host, port, net_connections, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // domain subcommand
   auto domain = app.add_subcommand("domain", localized("Create or update a domain"), false);
   domain.require_subcommand();
   
   set_domain_subcommands domain_subcommands(domain);

   // token subcommand
   auto token = app.add_subcommand("token", localized("Issue or transfer tokens"), false);
   token.require_subcommand();

   set_issue_token_subcommand issue_token(token);
   set_transfer_token_subcommand transfer(token);

   // sign subcommand
   string trx_json_to_sign;
   string str_private_key;
   bool push_trx = false;

   auto sign = app.add_subcommand("sign", localized("Sign a transaction"), false);
   sign->add_option("transaction", trx_json_to_sign,
                                 localized("The JSON of the transaction to sign, or the name of a JSON file containing the transaction"), true)->required();
   sign->add_option("-k,--private-key", str_private_key, localized("The private key that will be used to sign the transaction"));
   sign->add_flag( "-p,--push-transaction", push_trx, localized("Push transaction after signing"));

   sign->set_callback([&] {
      signed_transaction trx;
      if ( is_regular_file(trx_json_to_sign) ) {
         trx = fc::json::from_file(trx_json_to_sign).as<signed_transaction>();
      } else {
         trx = fc::json::from_string(trx_json_to_sign).as<signed_transaction>();
      }

      if( str_private_key.size() == 0 ) {
         std::cerr << localized("private key: ");
         fc::set_console_echo(false);
         std::getline( std::cin, str_private_key, '\n' );
         fc::set_console_echo(true);
      }

      auto priv_key = fc::crypto::private_key::regenerate(*utilities::wif_to_key(str_private_key));
      trx.sign(priv_key, chain_id_type{});

      if(push_trx) {
         auto trx_result = call(push_txn_func, packed_transaction(trx, packed_transaction::none));
         std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
      } else {
         std::cout << fc::json::to_pretty_string(trx) << std::endl;
      }
   });

   // Push subcommand
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"), false);
   push->require_subcommand();

   // push action
   string contract;
   string action;
   string data;
   vector<string> permissions;
   auto actionsSubcommand = push->add_subcommand("action", localized("Push a transaction with a single action"));
   actionsSubcommand->fallthrough(false);
   actionsSubcommand->add_option("contract", contract,
                                 localized("The account providing the contract to execute"), true)->required();
   actionsSubcommand->add_option("action", action, localized("The action to execute on the contract"), true)
         ->required();
   actionsSubcommand->add_option("data", data, localized("The arguments to the contract"))->required();

   add_standard_transaction_options(actionsSubcommand);
   actionsSubcommand->set_callback([&] {
      ilog("Converting argument to binary...");
      fc::variant action_args_var;
      try {
         action_args_var = fc::json::from_string(data);
      } EOS_CAPTURE_AND_RETHROW(action_type_exception, "Fail to parse action JSON")

      auto arg= fc::mutable_variant_object
                ("code", contract)
                ("action", action)
                ("args", action_args_var);
      auto result = call(json_to_bin_func, arg);

      auto accountPermissions = get_account_permissions(tx_permission);

      send_actions({chain::action{accountPermissions, contract, action, result.get_object()["binargs"].as<bytes>()}});
   });

   // push transaction
   string trx_to_push;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trx_to_push, localized("The JSON of the transaction to push, or the name of a JSON file containing the transaction"))->required();

   trxSubcommand->set_callback([&] {
      fc::variant trx_var;
      try {
         if ( is_regular_file(trx_to_push) ) {
            trx_var = fc::json::from_file(trx_to_push);
         } else {
            trx_var = fc::json::from_string(trx_to_push);
         }
      } EOS_CAPTURE_AND_RETHROW(transaction_type_exception, "Fail to parse transaction JSON")      signed_transaction trx = trx_var.as<signed_transaction>();
      auto trx_result = call(push_txn_func, packed_transaction(trx, packed_transaction::none));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localized("Push an array of arbitrary JSON transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localized("The JSON array of the transactions to push"))->required();
   trxsSubcommand->set_callback([&] {
      fc::variant trx_var;
      try {
         trx_var = fc::json::from_string(trxsJson);
      } EOS_CAPTURE_AND_RETHROW(transaction_type_exception, "Fail to parse transaction JSON")
      auto trxs_result = call(push_txns_func, trx_var);
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception& e) {
      return 1;
   } catch (const fc::exception& e) {
      auto errorString = e.to_detail_string();
      if (errorString.find("Connection refused") != string::npos) {
         if (errorString.find(fc::json::to_string(port)) != string::npos) {
            std::cerr << localized("Failed to connect to eosd at ${ip}:${port}; is eosd running?", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << localized("Failed to connect to eos-walletd at ${ip}:${port}; is eos-walletd running?", ("ip", wallet_host)("port", wallet_port)) << std::endl;
         } else {
            std::cerr << localized("Failed to connect") << std::endl;
         }

         if (verbose_errors) {
            elog("connect error: ${e}", ("e", errorString));
         }
      } else {
         // attempt to extract the error code if one is present
         if (!print_recognized_errors(e, verbose_errors)) {
            // Error is not recognized
            if (!print_help_text(e) || verbose_errors) {
               elog("Failed with error: ${e}", ("e", verbose_errors ? e.to_detail_string() : e.to_string()));
            }
         }
      }
      return 1;
   }

   return 0;
}
