/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <jmzk/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace jmzk {

using namespace appbase;

class trafficgen_plugin : public plugin<trafficgen_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    trafficgen_plugin()                         = default;
    trafficgen_plugin(const trafficgen_plugin&) = delete;
    trafficgen_plugin(trafficgen_plugin&&)      = delete;
    trafficgen_plugin& operator=(const trafficgen_plugin&) = delete;
    trafficgen_plugin& operator=(trafficgen_plugin&&) = delete;
    virtual ~trafficgen_plugin() = default;

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& vm);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::shared_ptr<class trafficgen_plugin_impl> my_;
};

}  // namespace jmzk
