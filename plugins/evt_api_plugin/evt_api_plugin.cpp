/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/exceptions.hpp>
#include <jmzk/jmzk_api_plugin/jmzk_api_plugin.hpp>

#include <fc/io/json.hpp>

namespace jmzk {

static appbase::abstract_plugin& _jmzk_api_plugin = app().register_plugin<jmzk_api_plugin>();

using namespace jmzk;

class jmzk_api_plugin_impl {
public:
    jmzk_api_plugin_impl(controller& db)
        : db(db) {}

    controller& db;
};

jmzk_api_plugin::jmzk_api_plugin() {}
jmzk_api_plugin::~jmzk_api_plugin() {}

void
jmzk_api_plugin::set_program_options(options_description&, options_description&) {}
void
jmzk_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code)                                              \
    {                                                                                                                         \
        std::string("/v1/" #api_name "/" #call_name),                                                                         \
            [api_handle](string, string body, url_response_callback cb) mutable {                                       \
                try {                                                                                                         \
                    if(body.empty())                                                                                          \
                        body = "{}";                                                                                          \
                    auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name##_params>());  \
                    cb(http_response_code, fc::json::to_string(result));                                                      \
                }                                                                                                             \
                catch (...) {                                                                                                 \
                    http_plugin::handle_exception(#api_name, #call_name, body, cb);                                           \
                }                                                                                                             \
            }                                                                                                                 \
    }

#define jmzk_RO_CALL(call_name, http_response_code) CALL(jmzk, ro_api, jmzk_apis::read_only, call_name, http_response_code)
#define jmzk_RW_CALL(call_name, http_response_code) CALL(jmzk, rw_api, jmzk_apis::read_write, call_name, http_response_code)

void
jmzk_api_plugin::plugin_startup() {
    ilog("starting jmzk_api_plugin");
    my.reset(new jmzk_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
    auto ro_api = app().get_plugin<jmzk_plugin>().get_read_only_api();

    app().get_plugin<http_plugin>().add_api({jmzk_RO_CALL(get_domain, 200),
                                             jmzk_RO_CALL(get_group, 200),
                                             jmzk_RO_CALL(get_token, 200),
                                             jmzk_RO_CALL(get_tokens, 200),
                                             jmzk_RO_CALL(get_fungible, 200),
                                             jmzk_RO_CALL(get_fungible_balance, 200),
                                             jmzk_RO_CALL(get_fungible_psvbonus, 200),
                                             jmzk_RO_CALL(get_suspend, 200),
                                             jmzk_RO_CALL(get_lock, 200),
                                             jmzk_RO_CALL(get_stakepool, 200),
                                             jmzk_RO_CALL(get_validator, 200),
                                             jmzk_RO_CALL(get_staking_shares, 200),
                                             jmzk_RO_CALL(get_jmzklink_signed_keys, 200),
                                             jmzk_RO_CALL(get_script, 200)
                                         });
}

void
jmzk_api_plugin::plugin_shutdown() {}

}  // namespace jmzk
