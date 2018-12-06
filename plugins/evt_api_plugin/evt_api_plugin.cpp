/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/exceptions.hpp>
#include <evt/evt_api_plugin/evt_api_plugin.hpp>

#include <fc/io/json.hpp>

namespace evt {

static appbase::abstract_plugin& _evt_api_plugin = app().register_plugin<evt_api_plugin>();

using namespace evt;

class evt_api_plugin_impl {
public:
    evt_api_plugin_impl(controller& db)
        : db(db) {}

    controller& db;
};

evt_api_plugin::evt_api_plugin() {}
evt_api_plugin::~evt_api_plugin() {}

void
evt_api_plugin::set_program_options(options_description&, options_description&) {}
void
evt_api_plugin::plugin_initialize(const variables_map&) {}

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

#define EVT_RO_CALL(call_name, http_response_code) CALL(evt, ro_api, evt_apis::read_only, call_name, http_response_code)
#define EVT_RW_CALL(call_name, http_response_code) CALL(evt, rw_api, evt_apis::read_write, call_name, http_response_code)

void
evt_api_plugin::plugin_startup() {
    ilog("starting evt_api_plugin");
    my.reset(new evt_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
    auto ro_api = app().get_plugin<evt_plugin>().get_read_only_api();

    app().get_plugin<http_plugin>().add_api({EVT_RO_CALL(get_domain, 200),
                                             EVT_RO_CALL(get_group, 200),
                                             EVT_RO_CALL(get_token, 200),
                                             EVT_RO_CALL(get_tokens, 200),
                                             EVT_RO_CALL(get_fungible, 200),
                                             EVT_RO_CALL(get_fungible_balance, 200),
                                             EVT_RO_CALL(get_suspend, 200),
                                             EVT_RO_CALL(get_lock, 200),
                                         });
}

void
evt_api_plugin::plugin_shutdown() {}

}  // namespace evt
