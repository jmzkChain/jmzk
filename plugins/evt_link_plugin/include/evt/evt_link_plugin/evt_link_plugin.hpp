/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/http_plugin/http_plugin.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>
#include <evt/chain/controller.hpp>

namespace evt {
using evt::chain::controller;
using namespace appbase;

class evt_link_plugin : public plugin<evt_link_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))

    evt_link_plugin();
    virtual ~evt_link_plugin();

    virtual void set_program_options(options_description&, options_description&) override;

    void plugin_initialize(const variables_map&);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::shared_ptr<class evt_link_plugin_impl> my_;
};

}  // namespace evt
