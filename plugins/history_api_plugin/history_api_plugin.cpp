/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/exceptions.hpp>
#include <evt/history_api_plugin/history_api_plugin.hpp>

#include <fc/io/json.hpp>

namespace evt {

static appbase::abstract_plugin& _history_api_plugin = app().register_plugin<history_api_plugin>();

using namespace evt;

class history_api_plugin_impl {
public:
    history_api_plugin_impl(controller& db)
        : db(db) {}

    controller& db;
};

history_api_plugin::history_api_plugin() {}
history_api_plugin::~history_api_plugin() {}

void
history_api_plugin::set_program_options(options_description&, options_description&) {}
void
history_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code)                                              \
    {                                                                                                                         \
        std::string("/v1/" #api_name "/" #call_name),                                                                         \
            [api_handle](string, string body, url_response_callback cb) mutable {                                             \
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

#define HISTORY_RO_CALL(call_name, http_response_code) CALL(history, ro_api, history_apis::read_only, call_name, http_response_code)

#define ASYNC_CALL(api_name, api_handle, api_namespace, call_name)                                                            \
    {                                                                                                                         \
        std::string("/v1/" #api_name "/" #call_name),                                                                         \
            [api_handle](string, string body, int id) mutable {                                                               \
                try {                                                                                                         \
                    if(body.empty())                                                                                          \
                        body = "{}";                                                                                          \
                    api_handle.call_name##_async(id, fc::json::from_string(body).as<api_namespace::call_name##_params>());    \
                }                                                                                                             \
                catch (...) {                                                                                                 \
                    http_plugin::handle_async_exception(id, #api_name, #call_name, body);                                     \
                }                                                                                                             \
            }                                                                                                                 \
    }

#define HISTORY_RO_ASYNC_CALL(call_name) ASYNC_CALL(history, ro_api, history_apis::read_only, call_name)

void
history_api_plugin::plugin_startup() {
    ilog("starting history_api_plugin");
    my.reset(new history_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
    auto ro_api = app().get_plugin<history_plugin>().get_read_only_api();

    app().get_plugin<http_plugin>().add_async_api({HISTORY_RO_ASYNC_CALL(get_tokens),
                                                   HISTORY_RO_ASYNC_CALL(get_domains),
                                                   HISTORY_RO_ASYNC_CALL(get_groups),
                                                   HISTORY_RO_ASYNC_CALL(get_fungibles),
                                                   HISTORY_RO_ASYNC_CALL(get_actions),
                                                   HISTORY_RO_ASYNC_CALL(get_fungible_actions),
                                                   HISTORY_RO_ASYNC_CALL(get_transaction),
                                                   HISTORY_RO_ASYNC_CALL(get_transactions),
                                                  });
}

void
history_api_plugin::plugin_shutdown() {}

}  // namespace evt
