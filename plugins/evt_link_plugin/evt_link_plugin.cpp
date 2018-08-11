/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/evt_link_plugin/evt_link_plugin.hpp>
#include <evt/chain/exceptions.hpp>
#include <fc/io/json.hpp>

namespace evt {

static appbase::abstract_plugin& _evt_link_plugin = app().register_plugin<evt_link_plugin>();

using namespace evt;

class evt_link_plugin_impl {
public:
    evt_link_plugin_impl(controller& db)
        : db(db) {}

    controller& db;
};

evt_link_plugin::evt_link_plugin() {}
evt_link_plugin::~evt_link_plugin() {}

void
evt_link_plugin::set_program_options(options_description&, options_description&) {}

void
evt_link_plugin::plugin_initialize(const variables_map&) {}

void
evt_link_plugin::plugin_startup() {
    ilog("starting evt_link_plugin");
    my.reset(new evt_link_plugin_impl(app().get_plugin<chain_plugin>().chain()));

    auto defer_id = 0u;
    app().get_plugin<http_plugin>().add_deferred_handler("/v1/evt_link/get_trx_id_for_link_id", [&](auto, auto body, auto id) {
        defer_id = id;

        auto t = std::thread([=] {
            ilog("sleeping, id: ${id}", ("id",defer_id));
            std::this_thread::sleep_for(std::chrono::seconds(5));
            ilog("sleeping - OK");

            app().get_plugin<http_plugin>().set_deferred_response(defer_id, 200, "hello world!!!");
        });
        t.detach();
    });
}

void
evt_link_plugin::plugin_shutdown() {}

}  // namespace evt
