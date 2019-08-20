/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/exceptions.hpp>
#include <evt/chain_api_plugin/chain_api_plugin.hpp>

#include <fc/io/json.hpp>

namespace evt {

static appbase::abstract_plugin& _chain_api_plugin = app().register_plugin<chain_api_plugin>();

using namespace evt;

class chain_api_plugin_impl {
public:
    chain_api_plugin_impl(controller& db)
        : db(db) {}

    controller& db;
};

chain_api_plugin::chain_api_plugin() {}
chain_api_plugin::~chain_api_plugin() {}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

struct async_result_visitor : public fc::visitor<std::string> {
    template <typename T>
    std::string
    operator()(const T& v) const {
        return fc::json::to_string(v);
    }
};

namespace internal {

template<typename T>
std::string
get_json(const T& value) {
    return fc::json::to_string(value);
}

template<>
std::string
get_json<std::string>(const std::string& value) {
    return value;
}

}  // namespace internal

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code)                                             \
    {                                                                                                                        \
        std::string("/v1/" #api_name "/" #call_name),                                                                        \
            [api_handle](string, string body, url_response_callback cb) mutable {                                            \
                using namespace internal;                                                                                  \
                try {                                                                                                        \
                    if(body.empty()) {                                                                                       \
                        body = "{}";                                                                                         \
                    }                                                                                                        \
                    auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name##_params>()); \
                    cb(http_response_code, get_json(result));                                                                \
                }                                                                                                            \
                catch(...) {                                                                                                 \
                    http_plugin::handle_exception(#api_name, #call_name, body, cb);                                          \
                }                                                                                                            \
            }                                                                                                                \
    }

#define CALL_ASYNC(api_name, api_handle, api_namespace, call_name, call_result, http_response_code)                 \
    {                                                                                                               \
        std::string("/v1/" #api_name "/" #call_name),                                                               \
            [api_handle](string, string body, url_response_callback cb) mutable {                                   \
                if(body.empty())                                                                                    \
                    body = "{}";                                                                                    \
                api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name##_params>(),           \
                                     [cb, body](const fc::static_variant<fc::exception_ptr, call_result>& result) { \
                                         if(result.contains<fc::exception_ptr>()) {                                 \
                                             try {                                                                  \
                                                 result.get<fc::exception_ptr>()->dynamic_rethrow_exception();      \
                                             }                                                                      \
                                             catch(...) {                                                           \
                                                 http_plugin::handle_exception(#api_name, #call_name, body, cb);    \
                                             }                                                                      \
                                         }                                                                          \
                                         else {                                                                     \
                                             cb(http_response_code, result.visit(async_result_visitor()));          \
                                         }                                                                          \
                                     });                                                                            \
            }                                                                                                       \
    }

#define CHAIN_RO_CALL(call_name, http_response_code) CALL(chain, ro_api, chain_apis::read_only, call_name, http_response_code)
#define CHAIN_RW_CALL(call_name, http_response_code) CALL(chain, rw_api, chain_apis::read_write, call_name, http_response_code)
#define CHAIN_RO_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(chain, ro_api, chain_apis::read_only, call_name, call_result, http_response_code)
#define CHAIN_RW_CALL_ASYNC(call_name, call_result, http_response_code) CALL_ASYNC(chain, rw_api, chain_apis::read_write, call_name, call_result, http_response_code)

void
chain_api_plugin::plugin_startup() {
    ilog("starting chain_api_plugin");
    my.reset(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
    auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();

    auto& _http_plugin = app().get_plugin<http_plugin>();
    ro_api.set_shorten_abi_errors(!_http_plugin.verbose_errors());

    _http_plugin.add_api({CHAIN_RO_CALL(get_info, 200),
                          CHAIN_RO_CALL(get_block, 200),
                          CHAIN_RO_CALL(get_block_header_state, 200),
                          CHAIN_RO_CALL(get_head_block_header_state, 200),
                          CHAIN_RO_CALL(get_transaction, 200),
                          CHAIN_RO_CALL(get_trx_id_for_link_id, 200),
                          CHAIN_RO_CALL(abi_json_to_bin, 200),
                          CHAIN_RO_CALL(abi_bin_to_json, 200),
                          CHAIN_RO_CALL(trx_json_to_digest, 200),
                          CHAIN_RO_CALL(trx_json_to_bin, 200),
                          CHAIN_RO_CALL(get_required_keys, 200),
                          CHAIN_RO_CALL(get_suspend_required_keys, 200),
                          CHAIN_RO_CALL(get_charge, 200),
                          CHAIN_RO_CALL(get_transaction_ids_for_block, 200),
                          CHAIN_RO_CALL(get_abi, 200),
                          CHAIN_RO_CALL(get_actions, 200),
                          CHAIN_RO_CALL(get_staking, 200),
                          CHAIN_RW_CALL_ASYNC(push_block, chain_apis::read_write::push_block_results, 202),
                          CHAIN_RW_CALL_ASYNC(push_transaction, chain_apis::read_write::push_transaction_results, 202),
                          CHAIN_RW_CALL_ASYNC(push_transactions, chain_apis::read_write::push_transactions_results, 202)});
    _http_plugin.add_api({CHAIN_RO_CALL(get_db_info, 200)}, true /* local only API */);
}

void
chain_api_plugin::plugin_shutdown() {}

}  // namespace evt
