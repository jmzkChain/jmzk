/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/chain/transaction.hpp>
#include <jmzk/wallet_api_plugin/wallet_api_plugin.hpp>
#include <jmzk/wallet_plugin/wallet_manager.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <chrono>

namespace jmzk { namespace detail {
struct wallet_api_plugin_empty {};
}}  // namespace jmzk::detail

FC_REFLECT(jmzk::detail::wallet_api_plugin_empty, );

namespace jmzk {

static appbase::abstract_plugin& _wallet_api_plugin = app().register_plugin<wallet_api_plugin>();

using namespace jmzk;

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code)                                                     \
    {                                                                                                                         \
        std::string("/v1/" #api_name "/" #call_name),                                                                         \
            [&api_handle](string, string body, url_response_callback cb) mutable {                                            \
                try {                                                                                                         \
                    if(body.empty())                                                                                          \
                        body = "{}";                                                                                          \
                    INVOKE                                                                                                    \
                    cb(http_response_code, fc::json::to_string(result));                                                      \
                }                                                                                                             \
                catch (...) {                                                                                                 \
                    http_plugin::handle_exception(#api_name, #call_name, body, cb);                                           \
                }                                                                                                             \
            }                                                                                                                 \
    }

#define INVOKE_R_R(api_handle, call_name, in_param) \
    auto result = api_handle.call_name(fc::json::from_string(body).as<in_param>());

#define INVOKE_R_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>());

#define INVOKE_R_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
    const auto& vs     = fc::json::json::from_string(body).as<fc::variants>(); \
    auto        result = api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>());

#define INVOKE_R_V(api_handle, call_name) \
    auto result = api_handle.call_name();

#define INVOKE_V_R(api_handle, call_name, in_param)                   \
    api_handle.call_name(fc::json::from_string(body).as<in_param>()); \
    jmzk::detail::wallet_api_plugin_empty result;

#define INVOKE_V_R_R(api_handle, call_name, in_param0, in_param1)             \
    const auto& vs = fc::json::json::from_string(body).as<fc::variants>();    \
    api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>()); \
    jmzk::detail::wallet_api_plugin_empty result;

#define INVOKE_V_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2)                          \
    const auto& vs = fc::json::json::from_string(body).as<fc::variants>();                              \
    api_handle.call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>()); \
    jmzk::detail::wallet_api_plugin_empty result;

#define INVOKE_V_V(api_handle, call_name) \
    api_handle.call_name();               \
    jmzk::detail::wallet_api_plugin_empty result;

void
wallet_api_plugin::plugin_startup() {
    ilog("starting wallet_api_plugin");
    // lifetime of plugin is lifetime of application
    auto& wallet_mgr = app().get_plugin<wallet_plugin>().get_wallet_manager();

    app().get_plugin<http_plugin>().add_api({CALL(wallet, wallet_mgr, set_timeout,
                                                  INVOKE_V_R(wallet_mgr, set_timeout, int64_t), 200),
                                             CALL(wallet, wallet_mgr, sign_transaction,
                                                  INVOKE_R_R_R_R(wallet_mgr, sign_transaction, chain::signed_transaction, flat_set<public_key_type>, chain::chain_id_type), 201),
                                             CALL(wallet, wallet_mgr, sign_digest,
                                                  INVOKE_R_R_R(wallet_mgr, sign_digest, chain::digest_type, public_key_type), 201),
                                             CALL(wallet, wallet_mgr, create,
                                                  INVOKE_R_R(wallet_mgr, create, std::string), 201),
                                             CALL(wallet, wallet_mgr, open,
                                                  INVOKE_V_R(wallet_mgr, open, std::string), 200),
                                             CALL(wallet, wallet_mgr, lock_all,
                                                  INVOKE_V_V(wallet_mgr, lock_all), 200),
                                             CALL(wallet, wallet_mgr, lock,
                                                  INVOKE_V_R(wallet_mgr, lock, std::string), 200),
                                             CALL(wallet, wallet_mgr, unlock,
                                                  INVOKE_V_R_R(wallet_mgr, unlock, std::string, std::string), 200),
                                             CALL(wallet, wallet_mgr, import_key,
                                                  INVOKE_V_R_R(wallet_mgr, import_key, std::string, std::string), 201),
                                             CALL(wallet, wallet_mgr, remove_key,
                                                  INVOKE_V_R_R_R(wallet_mgr, remove_key, std::string, std::string, std::string), 201),
                                             CALL(wallet, wallet_mgr, create_key,
                                                  INVOKE_R_R_R(wallet_mgr, create_key, std::string, std::string), 201),
                                             CALL(wallet, wallet_mgr, list_wallets,
                                                  INVOKE_R_V(wallet_mgr, list_wallets), 200),
                                             CALL(wallet, wallet_mgr, list_keys,
                                                  INVOKE_R_R_R(wallet_mgr, list_keys, std::string, std::string), 200),
                                             CALL(wallet, wallet_mgr, get_public_keys,
                                                  INVOKE_R_V(wallet_mgr, get_public_keys), 200),
                                             CALL(wallet, wallet_mgr, get_my_signatures,
                                                  INVOKE_R_R(wallet_mgr, get_my_signatures, chain::chain_id_type), 200)},
                                             !listen_http_ /* local only API */);
}

void
wallet_api_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("listen-http", boost::program_options::bool_switch()->default_value(false)->notifier(
            [this](bool l) { this->listen_http_ = l; }),
            "Wallet APIs are only listened on unix sockets defaultly, use this option also listen on http protocol.")
        ;
}

void
wallet_api_plugin::plugin_initialize(const variables_map& options) {}

#undef INVOKE_R_R
#undef INVOKE_R_R_R
#undef INVOKE_R_R_R_R
#undef INVOKE_R_V
#undef INVOKE_V_R
#undef INVOKE_V_R_R
#undef INVOKE_V_R_R_R
#undef INVOKE_V_V
#undef CALL

}  // namespace jmzk
