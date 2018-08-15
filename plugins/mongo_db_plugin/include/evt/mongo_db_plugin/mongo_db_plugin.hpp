/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <memory>

#include <appbase/application.hpp>
#include <mongocxx/client.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/types.hpp>

namespace evt {

using evt::chain::public_key_type;

class mongo_db_plugin : public plugin<mongo_db_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    mongo_db_plugin();
    virtual ~mongo_db_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

public:
    const mongocxx::uri& uri() const;
    bool  enabled() const;

private:
    std::unique_ptr<class mongo_db_plugin_impl> my_;
};

}  // namespace evt
