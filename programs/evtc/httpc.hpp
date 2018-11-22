/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>

namespace evt { namespace client { namespace http {

using std::string;
using std::vector;

namespace detail {
class http_context_impl;

struct http_context_deleter {
    void operator()(http_context_impl*) const;
};
}  // namespace detail

using http_context = std::unique_ptr<detail::http_context_impl, detail::http_context_deleter>;

http_context create_http_context();

struct parsed_url {
    string scheme;
    string server;
    string port;
    string path;

    static string normalize_path(const string& path);

    parsed_url
    operator+(const string& sub_path) {
        return {scheme, server, port, path + sub_path};
    }
};

parsed_url parse_url(const string& server_url);

struct resolved_url : parsed_url {
    resolved_url(const parsed_url& url, std::vector<string>&& resolved_addresses, uint16_t resolved_port, bool is_loopback)
        : parsed_url(url)
        , resolved_addresses(std::move(resolved_addresses))
        , resolved_port(resolved_port)
        , is_loopback(is_loopback) {
    }

    vector<string> resolved_addresses;
    uint16_t       resolved_port;
    bool           is_loopback;
};

resolved_url resolve_url(const http_context& context,
                         const parsed_url&   url);

struct connection_param {
    const http_context&  context;
    resolved_url         url;
    bool                 verify_cert;
    std::vector<string>& headers;

    connection_param(const http_context&  context,
                     const resolved_url&  url,
                     bool                 verify,
                     std::vector<string>& h)
        : context(context)
        , url(url)
        , headers(h) {
        verify_cert = verify;
    }

    connection_param(const http_context&  context,
                     const parsed_url&    url,
                     bool                 verify,
                     std::vector<string>& h)
        : context(context)
        , url(resolve_url(context, url))
        , headers(h) {
        verify_cert = verify;
    }
};

fc::variant do_http_call(
    const connection_param& cp,
    const fc::variant&      postdata       = fc::variant(),
    bool                    print_request  = false,
    bool                    print_response = false);

const std::string chain_func_base             = "/v1/chain";
const std::string get_info_func               = chain_func_base + "/get_info";
const std::string push_txn_func               = chain_func_base + "/push_transaction";
const std::string push_txns_func              = chain_func_base + "/push_transactions";
const std::string json_to_bin_func            = chain_func_base + "/abi_json_to_bin";
const std::string get_block_func              = chain_func_base + "/get_block";
const std::string get_block_header_state_func = chain_func_base + "/get_block_header_state";
const std::string get_transaction_func        = chain_func_base + "/get_transaction";
const std::string get_required_keys           = chain_func_base + "/get_required_keys";
const std::string get_suspend_required_keys   = chain_func_base + "/get_suspend_required_keys";
const std::string get_charge                  = chain_func_base + "/get_charge";

const std::string net_func_base   = "/v1/net";
const std::string net_connect     = net_func_base + "/connect";
const std::string net_disconnect  = net_func_base + "/disconnect";
const std::string net_status      = net_func_base + "/status";
const std::string net_connections = net_func_base + "/connections";

const std::string wallet_func_base     = "/v1/wallet";
const std::string wallet_create        = wallet_func_base + "/create";
const std::string wallet_open          = wallet_func_base + "/open";
const std::string wallet_list          = wallet_func_base + "/list_wallets";
const std::string wallet_list_keys     = wallet_func_base + "/list_keys";
const std::string wallet_public_keys   = wallet_func_base + "/get_public_keys";
const std::string wallet_lock          = wallet_func_base + "/lock";
const std::string wallet_lock_all      = wallet_func_base + "/lock_all";
const std::string wallet_unlock        = wallet_func_base + "/unlock";
const std::string wallet_import_key    = wallet_func_base + "/import_key";
const std::string wallet_remove_key    = wallet_func_base + "/remove_key";
const std::string wallet_create_key    = wallet_func_base + "/create_key";
const std::string wallet_sign_trx      = wallet_func_base + "/sign_transaction";

const std::string evt_func_base             = "/v1/evt";
const std::string get_domain_func           = evt_func_base + "/get_domain";
const std::string get_token_func            = evt_func_base + "/get_token";
const std::string get_group_func            = evt_func_base + "/get_group";
const std::string get_fungible_func         = evt_func_base + "/get_fungible";
const std::string get_fungible_balance_func = evt_func_base + "/get_fungible_balance";
const std::string get_suspend_func          = evt_func_base + "/get_suspend";
const std::string get_lock_func             = evt_func_base + "/get_lock";

const std::string history_func_base    = "/v1/history";
const std::string get_my_domains       = history_func_base + "/get_domains";
const std::string get_my_tokens        = history_func_base + "/get_tokens";
const std::string get_my_groups        = history_func_base + "/get_groups";
const std::string get_my_fungibles     = history_func_base + "/get_fungibles";
const std::string get_actions          = history_func_base + "/get_actions";
const std::string get_fungible_actions = history_func_base + "/get_fungible_actions";
const std::string get_transaction      = history_func_base + "/get_transaction";
const std::string get_transactions     = history_func_base + "/get_transactions";

const std::string producer_func_base    = "/v1/producer";
const std::string producer_pause        = producer_func_base + "/pause";
const std::string producer_resume       = producer_func_base + "/resume";
const std::string producer_paused       = producer_func_base + "/paused";
const std::string producer_runtime_opts = producer_func_base + "/get_runtime_options";
const std::string create_snapshot       = producer_func_base + "/create_snapshot";
const std::string get_integrity_hash    = producer_func_base + "/get_integrity_hash";


const string evtwd_stop = "/v1/evtwd/stop";

FC_DECLARE_EXCEPTION(connection_exception, 1100000, "Connection Exception");

}}}  // namespace evt::client::http