/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/trafficgen_plugin/trafficgen_plugin.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/transaction.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/plugin_interface.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/types.hpp>

namespace evt {

static appbase::abstract_plugin& _trafficgen_plugin = app().register_plugin<trafficgen_plugin>();

using evt::chain::block_state_ptr;

class trafficgen_plugin_impl : public std::enable_shared_from_this<trafficgen_plugin_impl> {
public:
    trafficgen_plugin_impl(controller& db)
        : db_(db) {}
    ~trafficgen_plugin_impl();

public:
    void init();

private:
    void applied_block(const block_state_ptr& bs);

public:
    controller& db_;

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection_;
};

void
trafficgen_plugin_impl::init() {
    auto& chain_plug = app().get_plugin<chain_plugin>();
    auto& chain      = chain_plug.chain();

    accepted_block_connection_.emplace(chain.accepted_block.connect([&](const chain::block_state_ptr& bs) {
        applied_block(bs);
    }));
}

void
trafficgen_plugin::set_program_options(options_description&, options_description& cfg) {}

void
trafficgen_plugin::plugin_initialize(const variables_map& options) {
    my_ = std::make_shared<trafficgen_plugin_impl>(app().get_plugin<chain_plugin>().chain());
    my_->init();
}

void
trafficgen_plugin::plugin_startup() {
    ilog("starting trafficgen_plugin");

}

void
trafficgen_plugin::plugin_shutdown() {
    my_->accepted_block_connection_.reset();
    my_.reset();
}

}  // namespace evt
