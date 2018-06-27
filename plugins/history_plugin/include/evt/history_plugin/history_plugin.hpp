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

#include <fc/optional.hpp>

namespace evt {

class history_plugin;
using evt::chain::public_key_type;
using evt::chain::action_name;

namespace history_apis {

class read_only {
public:
    read_only(const history_plugin& plugin)
        : plugin_(plugin) {}

public:
    struct get_params {
        std::vector<public_key_type> keys;
    };
    using get_tokens_params = get_params;
    using get_domains_params = get_params;
    using get_groups_params = get_params;

    fc::variant get_tokens(const get_params& params);
    fc::variant get_domains(const get_params& params);
    fc::variant get_groups(const get_params& params);

    struct get_actions_params {
        std::string               domain;
        fc::optional<std::string> key;
        std::vector<action_name>  names;
        fc::optional<int>         skip;
        fc::optional<int>         take;
    };
    fc::variant get_actions(const get_actions_params& params);

    struct get_transaction_params {
        chain::transaction_id_type id;
    };
    fc::variant get_transaction(const get_transaction_params& params);

    struct get_transactions_params {
        std::vector<public_key_type> keys;
        fc::optional<int>            skip;
        fc::optional<int>            take;        
    };
    fc::variant get_transactions(const get_transactions_params& params);


private:
    const history_plugin& plugin_;
};

}  // namespace history_apis

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

FC_REFLECT(evt::history_apis::read_only::get_params, (keys));
FC_REFLECT(evt::history_apis::read_only::get_actions_params, (domain)(key)(names)(skip)(take));
FC_REFLECT(evt::history_apis::read_only::get_transaction_params, (id));
FC_REFLECT(evt::history_apis::read_only::get_transactions_params, (keys)(skip)(take));
