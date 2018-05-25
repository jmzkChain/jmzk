/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <string>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>

namespace evt { namespace client { namespace http {

using std::string;

struct connection_param {
    string& url;
    string& path;
    bool verify_cert;
    std::vector<string>& headers;

    connection_param( std::string& u,
                     std::string& p,
                     bool verify,
                     std::vector<string>& h) : url(u), path(p), headers(h) {
       verify_cert = verify;
    }
};

struct parsed_url {
    std::string scheme;
    std::string server;
    std::string port;
    std::string path_prefix;
};

parsed_url parse_url(const std::string& server_url);

fc::variant do_http_call(const connection_param& cp,
                         const fc::variant& postdata = fc::variant());

const std::string chain_func_base   = "/v1/chain";
const std::string get_info_func     = chain_func_base + "/get_info";
const std::string push_txn_func     = chain_func_base + "/push_transaction";
const std::string push_txns_func    = chain_func_base + "/push_transactions";
const std::string json_to_bin_func  = chain_func_base + "/abi_json_to_bin";
const std::string get_block_func    = chain_func_base + "/get_block";
const std::string get_required_keys = chain_func_base + "/get_required_keys";

const std::string account_history_func_base    = "/v1/account_history";
const std::string get_transaction_func         = account_history_func_base + "/get_transaction";
const std::string get_transactions_func        = account_history_func_base + "/get_transactions";
const std::string get_key_accounts_func        = account_history_func_base + "/get_key_accounts";
const std::string get_controlled_accounts_func = account_history_func_base + "/get_controlled_accounts";

const std::string net_func_base   = "/v1/net";
const std::string net_connect     = net_func_base + "/connect";
const std::string net_disconnect  = net_func_base + "/disconnect";
const std::string net_status      = net_func_base + "/status";
const std::string net_connections = net_func_base + "/connections";

const std::string wallet_func_base   = "/v1/wallet";
const std::string wallet_create      = wallet_func_base + "/create";
const std::string wallet_open        = wallet_func_base + "/open";
const std::string wallet_list        = wallet_func_base + "/list_wallets";
const std::string wallet_list_keys   = wallet_func_base + "/list_keys";
const std::string wallet_public_keys = wallet_func_base + "/get_public_keys";
const std::string wallet_lock        = wallet_func_base + "/lock";
const std::string wallet_lock_all    = wallet_func_base + "/lock_all";
const std::string wallet_unlock      = wallet_func_base + "/unlock";
const std::string wallet_import_key  = wallet_func_base + "/import_key";
const std::string wallet_sign_trx    = wallet_func_base + "/sign_transaction";

const std::string evt_func_base       = "/v1/evt";
const std::string new_domain_func     = evt_func_base + "/new_domain";
const std::string issue_token_func    = evt_func_base + "/issue_token";
const std::string transfer_token_func = evt_func_base + "/transfer";
const std::string update_group_func   = evt_func_base + "/update_group";
const std::string update_domain_func  = evt_func_base + "/update_domain";

const std::string get_domain_func           = evt_func_base + "/get_domain";
const std::string get_token_func            = evt_func_base + "/get_token";
const std::string get_group_func            = evt_func_base + "/get_group";
const std::string get_account_func          = evt_func_base + "/get_account";
const std::string get_currency_balance_func = evt_func_base + "/get_currency_balance";
const std::string get_currency_stats_func   = evt_func_base + "/get_currency_stats";

FC_DECLARE_EXCEPTION(connection_exception, 1100000, "Connection Exception");
}}}  // namespace evt::client::http