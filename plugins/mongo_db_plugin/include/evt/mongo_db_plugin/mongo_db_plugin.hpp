/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <appbase/application.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/types.hpp>
#include <fc/container/flat_fwd.hpp>
#include <memory>

namespace evt {

using mongo_db_plugin_impl_ptr = std::shared_ptr<class mongo_db_plugin_impl>;
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
    fc::flat_set<std::string> get_tokens_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_domains_by_public_keys(const std::vector<public_key_type>& pkeys);
    fc::flat_set<std::string> get_groups_by_public_keys(const std::vector<public_key_type>& pkeys);

private:
    mongo_db_plugin_impl_ptr my;
};

}  // namespace evt
