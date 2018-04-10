/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <string>
#include <vector>
#include <regex>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/io/fstream.hpp>
#include <fc/exception/exception.hpp>

#include <evt/utilities/key_conversion.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/contracts/types.hpp>
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
using namespace evt;
using namespace evt::chain;
using namespace evt::chain::contracts;
using namespace evt::utilities;
using namespace evt::client::help;
using namespace evt::client::http;
using namespace evt::client::localize;
using namespace evt::client::config;
using namespace boost::filesystem;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );
FC_DECLARE_EXCEPTION( localized_exception, 10000000, "an error occured" );
#define EVTC_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                   \
        std::cerr << localized( __VA_ARGS__ ) << std::endl;  \
        FC_THROW_EXCEPTION( explained_exception, #TEST ); \
      }                                                   \
    FC_MULTILINE_MACRO_END \
  )

string program = "evtc";
string host = "localhost";
uint32_t port = 8888;

// restricting use of wallet to localhost
string wallet_host = "localhost";
uint32_t wallet_port = 9999;

auto   tx_expiration = fc::seconds(30);
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
   cmd->add_flag("-s,--skip-sign", tx_skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   cmd->add_flag("-d,--dont-broadcast", tx_dont_broadcast, localized("don't broadcast transaction to the network (just print to stdout)"));

   string msg = "An account and permission level to authorize, as in 'account@permission'";
   if(!default_permission.empty())
      msg += " (defaults to '" + default_permission + "')";
   cmd->add_option("-p,--permission", tx_permission, localized(msg.c_str()));
}

template<typename T>
fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const T& v ) { return evt::client::http::call( server, port, path, fc::variant(v) ); }

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return evt::client::http::call( host, port, path, fc::variant(v) ); }

evt::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<evt::chain_apis::read_only::get_info_results>();
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

fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser)
{
   regex r("^[ \t]*[\{\[]");
   if ( !regex_search(file_or_str, r) && fc::is_regular_file(file_or_str) ) {
      return fc::json::from_file(file_or_str, ptype);
   } else {
      return fc::json::from_string(file_or_str, ptype);
   }
}

auto parse_permission = [](auto& jsonOrFile) {
  try {
    auto parsedPermission = json_from_file_or_string(jsonOrFile);
    permission_def permission;
    parsedPermission.as(permission);
    return permission;
  } EVT_RETHROW_EXCEPTIONS(permission_type_exception, "Fail to parse Permission JSON")
};

auto parse_groups = [](auto& jsonOrFile) {
  try {
    auto parsedGroups = json_from_file_or_string(jsonOrFile);
    std::vector<group_def> groups;
    parsedGroups.as(groups);
    return groups;
  } EVT_RETHROW_EXCEPTIONS(groups_type_exception, "Fail to parse Groups JSON")
};

struct set_domain_subcommands {
   string name;
   string issuer;
   string issue;
   string transfer;
   string manage;
   string groups;

   set_domain_subcommands(CLI::App* actionRoot) {
      auto ndcmd = actionRoot->add_subcommand("new", localized("Add new domain"));
      ndcmd->add_option("name", name, localized("The name of new domain"))->required();
      ndcmd->add_option("issuer", issuer, localized("The public key of the issuer"))->required();
      ndcmd->add_option("issue", issue, localized("JSON string or filename defining ISSUE permission"))->required();
      ndcmd->add_option("transfer", transfer, localized("JSON string or filename defining TRANSFER permission"))->required();
      ndcmd->add_option("manage", manage, localized("JSON string or filename defining MANAGE permission"))->set_default_str("{}");
      ndcmd->add_option("groups", groups, localized("JSON string or filename defining groups which are new defined"))->set_default_str("[]");

      add_standard_transaction_options(ndcmd);

      ndcmd->set_callback([&] {
         newdomain nd;
         nd.name = name128(name);
         nd.issuer = public_key_type(issuer);
         nd.issue = parse_permission(issue);
         nd.transfer = parse_permission(transfer);
         nd.manage = parse_permission(manage);
         nd.groups = parse_groups(groups);
         
         auto act = create_action("domain", (domain_key)nd.name, nd);
         send_actions({ act });
      });
      
      auto udcmd = actionRoot->add_subcommand("update", localized("Update existing domain"));
      udcmd->add_option("name", name, localized("The name of new domain"))->required();
      udcmd->add_option("issue", issue, localized("JSON string or filename defining ISSUE permission"))->required();
      udcmd->add_option("transfer", transfer, localized("JSON string or filename defining TRANSFER permission"))->required();
      udcmd->add_option("manage", manage, localized("JSON string or filename defining MANAGE permission"))->required();
      udcmd->add_option("groups", groups, localized("JSON string or filename defining groups which are new defined"))->required();

      add_standard_transaction_options(udcmd);

      udcmd->set_callback([&] {
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
      auto itcmd = actionRoot->add_subcommand("issue", localized("Issue new tokens in specific domain"));
      itcmd->add_option("domain", domain, localized("Name of the domain where token issued"))->required();
      itcmd->add_option("-n,--names", names, localized("Names of tokens will be issued"))->required();
      itcmd->add_option("owner", owner, localized("Owner that issued tokens belongs to"))->required();

      add_standard_transaction_options(itcmd);

      itcmd->set_callback([this] {
         issuetoken it;
         it.domain = name128(domain);
         std::transform(names.cbegin(), names.cend(), std::back_inserter(it.names), [](auto& str) { return name128(str); });
         std::transform(owner.cbegin(), owner.cend(), std::back_inserter(it.owner), [](auto& str) { return public_key_type(str); });

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
      auto ttcmd = actionRoot->add_subcommand("transfer", localized("Transfer token"));
      ttcmd->add_option("domain", domain, localized("Name of the domain where token existed"))->required();
      ttcmd->add_option("name", name, localized("Name of the token to be transfered"))->required();
      ttcmd->add_option("to", to, localized("User list receives this token"))->required();
      
      add_standard_transaction_options(ttcmd);

      ttcmd->set_callback([this] {
         transfer tt;
         tt.domain = name128(domain);
         tt.name = name128(name);
         std::transform(to.cbegin(), to.cend(), std::back_inserter(tt.to), [](auto& str) { return public_key_type(str); });

         auto act = create_action(tt.domain, (domain_key)tt.name, tt);
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
      auto ugcmd = actionRoot->add_subcommand("update", localized("Update specific permission group, id or key must provide at least one."));
      ugcmd->add_option("id", id, localized("Id of the permission group to be updated"));
      ugcmd->add_option("-k,--key", key, localized("Key of permission group to be updated"));
      ugcmd->add_option("threshold", threshold, localized("Threshold of permission group"))->required();
      ugcmd->add_option("keys", keys, localized("JSON string or filename defining the keys of permission group"))->required();

      add_standard_transaction_options(ugcmd);

      ugcmd->set_callback([this] {
         updategroup ug;
         group_id gid;
         FC_ASSERT(!(id.empty() && key.empty()), "Must provide either id or key");
         if(!id.empty()) {
             try {
                gid = group_id::from_base58(id);
             } FC_CAPTURE_AND_RETHROW((id))
         }
         if(!key.empty()) {
             try {
                gid = group_id::from_group_key(public_key_type(key));
                printf("Group id: %s\n", gid.to_base58().c_str());
             } FC_CAPTURE_AND_RETHROW((key))
         }
         ug.id = gid;
         ug.threshold = threshold;

         try {
            auto parsedKeys = json_from_file_or_string(keys);
            parsedKeys.as(ug.keys);
         } EVT_RETHROW_EXCEPTIONS(group_keys_type_exception, "Fail to parse Group Keys JSON")

         auto act = create_action("group", (domain_key)ug.id, ug);
         send_actions({ act });
      });

      auto gicmd = actionRoot->add_subcommand("getid", localized("Get group id from group key"));
      gicmd->add_option("key", key, localized("Group key to be converted"))->required();
      gicmd->set_callback([this] {
          try {
              auto gid = group_id::from_group_key(public_key_type(key));
              printf("Group id: %s\n", gid.to_base58().c_str());
          } FC_CAPTURE_AND_RETHROW((key))
      });
   }
};

struct set_get_domain_subcommand {
   string name;

   set_get_domain_subcommand(CLI::App* actionRoot) {
       auto gdcmd = actionRoot->add_subcommand("domain", localized("Retrieve a domain information"));
       gdcmd->add_option("name", name, localized("Name of domain to be retrieved"))->required();

       gdcmd->set_callback([this] {
          auto arg = fc::mutable_variant_object("name", name);
          std::cout << fc::json::to_pretty_string(call(get_domain_func, arg)) << std::endl;
       });
   }
};

struct set_get_token_subcommand {
   string domain;
   string name;

   set_get_token_subcommand(CLI::App* actionRoot) {
       auto gtcmd = actionRoot->add_subcommand("token", localized("Retrieve a token information"));
       gtcmd->add_option("domain", domain, localized("Domain name of token to be retrieved"))->required();
       gtcmd->add_option("name", name, localized("Name of token to be retrieved"))->required();

       gtcmd->set_callback([this] {
          auto arg = fc::mutable_variant_object();
          arg.set("domain", domain);
          arg.set("name", name);
          std::cout << fc::json::to_pretty_string(call(get_token_func, arg)) << std::endl;
       });
   }
};

struct set_get_group_subcommand {
   string id;
   string key;

   set_get_group_subcommand(CLI::App* actionRoot) {
       auto ggcmd = actionRoot->add_subcommand("group", localized("Retrieve a permission group information"));
       ggcmd->add_option("id,-i,--id", id, localized("Id of group to be retrieved"));
       ggcmd->add_option("-k,--key", key, localized("Key of group to be retrieved"));

       ggcmd->set_callback([this] {
          group_id gid;
          FC_ASSERT(!(id.empty() && key.empty()), "Must provide either id or key");
          if(!id.empty()) {
             try {
                gid = group_id::from_base58(id);
             } FC_CAPTURE_AND_RETHROW((id))
          }
          if(!key.empty()) {
             try {
                gid = group_id::from_group_key(public_key_type(key));
                printf("Group id: %s\n", gid.to_base58().c_str());
             } FC_CAPTURE_AND_RETHROW((key))
          }

          auto arg = fc::mutable_variant_object("id", gid.to_base58());
          std::cout << fc::json::to_pretty_string(call(get_group_func, arg)) << std::endl;
       });
   }
};

int main( int argc, char** argv ) {
   fc::path binPath = argv[0];
   if (binPath.is_relative()) {
      binPath = relative(binPath, fc::current_path()); 
   }

   setlocale(LC_ALL, "");
   bindtextdomain(locale_domain, locale_path);
   textdomain(locale_domain);

   CLI::App app{"Command Line Interface to Eos Client"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localized("the host where evtd is running"), true );
   app.add_option( "-p,--port", port, localized("the port where evtd is running"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, localized("output verbose actions on error"));

   auto version = app.add_subcommand("version", localized("Retrieve version information"));
   version->require_subcommand();

   version->add_subcommand("client", localized("Retrieve version information of the client"))->set_callback([] {
     std::cout << localized("Build version: ${ver}", ("ver", evt::client::config::version_str)) << std::endl;
   });

   // Create subcommand
   auto create = app.add_subcommand("create", localized("Create various items, on and off the blockchain"));
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
   auto get = app.add_subcommand("get", localized("Retrieve various items and information from the blockchain"));
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localized("Get current blockchain information"))->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", localized("Retrieve a full block from the blockchain"));
   getBlock->add_option("block", blockArg, localized("The number or ID of the block to retrieve"))->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"));
   getTransaction->add_option("id", transactionId, localized("ID of the transaction to retrieve"))->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string account_name;
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"));
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
   auto net = app.add_subcommand( "net", localized("Interact with local p2p network connections") );
   net->require_subcommand();
   auto connect = net->add_subcommand("connect", localized("start a new connection to a peer"));
   connect->add_option("host", new_host, localized("The hostname:port to connect to."))->required();
   connect->set_callback([&] {
      const auto& v = call(host, port, net_connect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto disconnect = net->add_subcommand("disconnect", localized("close an existing connection"));
   disconnect->add_option("host", new_host, localized("The hostname:port to disconnect from."))->required();
   disconnect->set_callback([&] {
      const auto& v = call(host, port, net_disconnect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto status = net->add_subcommand("status", localized("status of existing connection"));
   status->add_option("host", new_host, localized("The hostname:port to query status of connection"))->required();
   status->set_callback([&] {
      const auto& v = call(host, port, net_status, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto connections = net->add_subcommand("peers", localized("status of all existing peers"));
   connections->set_callback([&] {
      const auto& v = call(host, port, net_connections, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // domain subcommand
   auto domain = app.add_subcommand("domain", localized("Create or update a domain"));
   domain->require_subcommand();
   
   auto set_domain = set_domain_subcommands(domain);

   // token subcommand
   auto token = app.add_subcommand("token", localized("Issue or transfer tokens"));
   token->require_subcommand();

   auto set_issue_token = set_issue_token_subcommand(token);
   auto set_transfer_token = set_transfer_token_subcommand(token);

   // group subcommand
   auto group = app.add_subcommand("group", localized("Update pemission group"));
   group->require_subcommand();
   
   auto set_group = set_group_subcommands(group);

   // Wallet subcommand
   auto wallet = app.add_subcommand( "wallet", localized("Interact with local wallet") );
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   auto createWallet = wallet->add_subcommand("create", localized("Create a new wallet locally"));
   createWallet->add_option("-n,--name", wallet_name, localized("The name of the new wallet"));
   createWallet->set_callback([&wallet_name] {
      const auto& v = call(wallet_host, wallet_port, wallet_create, wallet_name);
      std::cout << localized("Creating wallet: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      std::cout << localized("Save password to use in the future to unlock this wallet.") << std::endl;
      std::cout << localized("Without password imported keys will not be retrievable.") << std::endl;
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", localized("Open an existing wallet"));
   openWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to open"));
   openWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_open, wallet_name);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localized("Opened: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
   });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", localized("Lock wallet"));
   lockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to lock"));
   lockWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock, wallet_name);
      std::cout << localized("Locked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;

   });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", localized("Lock all unlocked wallets"));
   locakAllWallets->set_callback([] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock_all);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localized("Locked All Wallets") << std::endl;
   });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", localized("Unlock wallet"));
   unlockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to unlock"));
   unlockWallet->add_option("--password", wallet_pw, localized("The password returned by wallet create"));
   unlockWallet->set_callback([&wallet_name, &wallet_pw] {
      if( wallet_pw.size() == 0 ) {
         std::cout << localized("password: ");
         fc::set_console_echo(false);
         std::getline( std::cin, wallet_pw, '\n' );
         fc::set_console_echo(true);
      }


      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      /*const auto& v = */call(wallet_host, wallet_port, wallet_unlock, vs);
      std::cout << localized("Unlocked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // import keys into wallet
   string wallet_key_str;
   auto importWallet = wallet->add_subcommand("import", localized("Import private key into wallet"));
   importWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to import key into"));
   importWallet->add_option("key", wallet_key_str, localized("Private key in WIF format to import"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key_str] {
      private_key_type wallet_key;
      try {
         wallet_key = private_key_type( wallet_key_str );
      } catch (...) {
          EVT_THROW(private_key_type_exception, "Invalid private key: ${private_key}", ("private_key", wallet_key_str))
      }
      public_key_type pubkey = wallet_key.get_public_key();

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_key)};
      const auto& v = call(wallet_host, wallet_port, wallet_import_key, vs);
      std::cout << localized("imported private key for: ${pubkey}", ("pubkey", std::string(pubkey))) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", localized("List opened wallets, * = unlocked"));
   listWallet->set_callback([] {
      std::cout << localized("Wallets:") << std::endl;
      const auto& v = call(wallet_host, wallet_port, wallet_list);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", localized("List of private keys from all unlocked wallets in wif format."));
   listKeys->set_callback([] {
      const auto& v = call(wallet_host, wallet_port, wallet_list_keys);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // sign subcommand
   string trx_json_to_sign;
   string str_private_key;
   bool push_trx = false;

   auto sign = app.add_subcommand("sign", localized("Sign a transaction"));
   sign->add_option("transaction", trx_json_to_sign,
                                 localized("The JSON of the transaction to sign, or the name of a JSON file containing the transaction"), true)->required();
   sign->add_option("-k,--private-key", str_private_key, localized("The private key that will be used to sign the transaction"));
   sign->add_flag( "-p,--push-transaction", push_trx, localized("Push transaction after signing"));

   sign->set_callback([&] {
      signed_transaction trx;
      if ( fc::is_regular_file(trx_json_to_sign) ) {
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
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"));
   push->require_subcommand();

   // push transaction
   string trx_to_push;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trx_to_push, localized("The JSON of the transaction to push, or the name of a JSON file containing the transaction"))->required();

   trxSubcommand->set_callback([&] {
      fc::variant trx_var;
      try {
         if ( fc::is_regular_file(trx_to_push) ) {
            trx_var = fc::json::from_file(trx_to_push);
         } else {
            trx_var = fc::json::from_string(trx_to_push);
         }
      } EVT_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON")
      signed_transaction trx = trx_var.as<signed_transaction>();
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
      } EVT_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON")
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
            std::cerr << localized("Failed to connect to evtd at ${ip}:${port}; is evtd running?", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << localized("Failed to connect to evt-walletd at ${ip}:${port}; is evt-walletd running?", ("ip", wallet_host)("port", wallet_port)) << std::endl;
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
