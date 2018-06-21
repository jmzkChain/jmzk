/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>

#include <appbase/application.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/mongo_db_plugin/mongo_db_plugin.hpp>

#include <evt/chain/controller.hpp>
#include <evt/chain/types.hpp>
#include <fc/container/flat_fwd.hpp>

namespace evt {

class history_plugin;


namespace history_apis {

class read_only {
public:
    read_only(const history_plugin& plugin)
        : plugin_(plugin) {}

public:
    struct get_my_params {
        std::vector<std::string> signatures;
    };
    using get_my_tokens_params = get_my_params;
    using get_my_domains_params = get_my_params;
    using get_my_groups_params = get_my_params;

    fc::variant get_my_tokens(const get_my_params& params);
    fc::variant get_my_domains(const get_my_params& params);
    fc::variant get_my_groups(const get_my_params& params);

private:
    const history_plugin& plugin_;
};

}  // namespace history_apis

using evt::chain::public_key_type;

class history_plugin : public plugin<history_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(mongo_db_plugin))

    history_plugin();
    virtual ~history_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

public:
    history_apis::read_only get_read_only_api() const { return history_apis::read_only(*this); }

private:
    std::unique_ptr<class history_plugin_impl> my_;
    friend class history_apis::read_only;
};

}  // namespace evt

FC_REFLECT(evt::history_apis::read_only::get_my_params, (signatures));