/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>

namespace fc {
class variant;
}

namespace evt {

using namespace appbase;

using std::shared_ptr;
using std::optional;

using chain::name;
using chain::uint128_t;
using chain::transaction_id_type;

typedef shared_ptr<class bnet_plugin_impl>       bnet_ptr;
typedef shared_ptr<const class bnet_plugin_impl> bnet_const_ptr;

class bnet_plugin : public plugin<bnet_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    bnet_plugin();
    virtual ~bnet_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();
    void handle_sighup() override;

private:
    bnet_ptr my;
};

}  // namespace evt
