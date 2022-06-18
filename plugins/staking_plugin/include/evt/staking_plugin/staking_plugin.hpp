/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/chain_plugin/chain_plugin.hpp>
#include <jmzk/http_client_plugin/http_client_plugin.hpp>

#include <appbase/application.hpp>
#include <jmzk/chain/controller.hpp>

namespace jmzk {
using jmzk::chain::controller;
using namespace appbase;

class staking_plugin : public plugin<staking_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_client_plugin))

    staking_plugin();
    virtual ~staking_plugin();

    virtual void set_program_options(options_description&, options_description&) override;

    void plugin_initialize(const variables_map&);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::shared_ptr<class staking_plugin_impl> my_;
};

}  // namespace jmzk
