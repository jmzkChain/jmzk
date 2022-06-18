/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/history_plugin/history_plugin.hpp>
#include <jmzk/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>
#include <jmzk/chain/controller.hpp>

namespace jmzk {
using jmzk::chain::controller;
using std::unique_ptr;
using namespace appbase;

class history_api_plugin : public plugin<history_api_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin)(history_plugin))

    history_api_plugin();
    virtual ~history_api_plugin();

    virtual void set_program_options(options_description&, options_description&) override;

    void plugin_initialize(const variables_map&);
    void plugin_startup();
    void plugin_shutdown();

private:
    unique_ptr<class history_api_plugin_impl> my;
};

}  // namespace jmzk
