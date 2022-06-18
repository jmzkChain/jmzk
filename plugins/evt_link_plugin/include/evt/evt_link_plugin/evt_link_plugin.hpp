/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <jmzk/http_plugin/http_plugin.hpp>
#include <jmzk/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>
#include <jmzk/chain/controller.hpp>

namespace jmzk {
using jmzk::chain::controller;
using namespace appbase;

class jmzk_link_plugin : public plugin<jmzk_link_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))

    jmzk_link_plugin();
    virtual ~jmzk_link_plugin();

    virtual void set_program_options(options_description&, options_description&) override;

    void plugin_initialize(const variables_map&);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::shared_ptr<class jmzk_link_plugin_impl> my_;
};

}  // namespace jmzk
