/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

namespace eosio { namespace client { namespace http {
   fc::variant call( const std::string& server, uint16_t port,
                     const std::string& path,
                     const fc::variant& postdata = fc::variant() );

   const string chain_func_base = "/v1/chain";
   const string get_info_func = chain_func_base + "/get_info";
   const string push_txn_func = chain_func_base + "/push_transaction";
   const string push_txns_func = chain_func_base + "/push_transactions";
   const string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
   const string get_block_func = chain_func_base + "/get_block";
   const string get_required_keys = chain_func_base + "/get_required_keys";

   const string net_func_base = "/v1/net";
   const string net_connect = net_func_base + "/connect";
   const string net_disconnect = net_func_base + "/disconnect";
   const string net_status = net_func_base + "/status";
   const string net_connections = net_func_base + "/connections";

   const string evt_func_base = "/v1/evt";
   const string new_domain_func = evt_func_base + "/new_domain";
   const string issue_token_func = evt_func_base + "/issue_token";
   const string transfer_token_func = evt_func_base + "/transfer";
   const string update_group_func = evt_func_base + "/update_group";
   const string update_domain_func = evt_func_base + "/update_domain";

   const string get_domain_func = evt_func_base + "/get_domain";
   const string get_token_func = evt_func_base + "/get_token";
   const string get_group_func = evt_func_base + "/get_group";
 }}}