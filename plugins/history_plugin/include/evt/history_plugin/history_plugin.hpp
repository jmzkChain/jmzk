/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <memory>

#include <appbase/application.hpp>
#include <fc/optional.hpp>

#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/postgres_plugin/postgres_plugin.hpp>
#include <evt/chain/types.hpp>

namespace evt {

class history_plugin;

using evt::chain::address;
using evt::chain::action_name;
using evt::chain::domain_name;
using evt::chain::public_key_type;
using evt::chain::symbol_id_type;
using evt::chain::token_name;
using evt::chain::transaction_id_type;

namespace history_apis {

enum class direction : uint8_t {
    desc = 0, asc
};

class read_only {
public:
    read_only(const history_plugin& plugin)
        : plugin_(plugin) {}

public:
    struct get_tokens_params {
        std::vector<public_key_type> keys;
        fc::optional<domain_name>    domain;
    };
    void get_tokens_async(int id, const get_tokens_params& params);

    struct get_params {
        std::vector<public_key_type> keys;
    };
    using get_domains_params = get_params;
    using get_groups_params = get_params;
    using get_fungibles_params = get_params;

    void get_domains_async(int id, const get_params& params);
    void get_groups_async(int id, const get_params& params);
    void get_fungibles_async(int id, const get_params& params);

    struct get_actions_params {
        std::string                                     domain;
        fc::optional<std::string>                       key;
        std::vector<action_name>                        names;
        fc::optional<fc::enum_type<uint8_t, direction>> dire;
        fc::optional<int>                               skip;
        fc::optional<int>                               take;
    };
    void get_actions_async(int id, const get_actions_params& params);

    struct get_fungible_actions_params {
        symbol_id_type                                  sym_id;
        fc::optional<fc::enum_type<uint8_t, direction>> dire;
        fc::optional<address>                           addr;
        fc::optional<int>                               skip;
        fc::optional<int>                               take; 
    };
    void get_fungible_actions_async(int id, const get_fungible_actions_params& params);

    struct get_transaction_params {
        transaction_id_type id;
    };
    void get_transaction_async(int id, const get_transaction_params& params);

    struct get_transactions_params {
        std::vector<public_key_type>                    keys;
        fc::optional<fc::enum_type<uint8_t, direction>> dire;
        fc::optional<int>                               skip;
        fc::optional<int>                               take;
    };
    void get_transactions_async(int id, const get_transactions_params& params);

private:
    const history_plugin& plugin_;
};

}  // namespace history_apis

class history_plugin : public plugin<history_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(postgres_plugin))

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

FC_REFLECT_ENUM(evt::history_apis::direction, (asc)(desc));
FC_REFLECT(evt::history_apis::read_only::get_params, (keys));
FC_REFLECT(evt::history_apis::read_only::get_tokens_params, (keys)(domain));
FC_REFLECT(evt::history_apis::read_only::get_actions_params, (domain)(key)(dire)(names)(skip)(take));
FC_REFLECT(evt::history_apis::read_only::get_fungible_actions_params, (sym_id)(dire)(addr)(skip)(take));
FC_REFLECT(evt::history_apis::read_only::get_transaction_params, (id));
FC_REFLECT(evt::history_apis::read_only::get_transactions_params, (keys)(dire)(skip)(take));
