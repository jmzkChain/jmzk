/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <memory>
#include <optional>

#include <appbase/application.hpp>

#include <jmzk/chain_plugin/chain_plugin.hpp>
#include <jmzk/postgres_plugin/postgres_plugin.hpp>
#include <jmzk/chain/types.hpp>

namespace jmzk {

class history_plugin;

using jmzk::chain::address;
using jmzk::chain::action_name;
using jmzk::chain::domain_name;
using jmzk::chain::public_key_type;
using jmzk::chain::symbol_id_type;
using jmzk::chain::token_name;
using jmzk::chain::transaction_id_type;

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
        optional<domain_name>        domain;
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
        std::string                                 domain;
        optional<std::string>                       key;
        std::vector<action_name>                    names;
        optional<fc::enum_type<uint8_t, direction>> dire;
        optional<int>                               skip;
        optional<int>                               take;
    };
    void get_actions_async(int id, const get_actions_params& params);

    struct get_fungible_actions_params {
        symbol_id_type                              sym_id;
        optional<fc::enum_type<uint8_t, direction>> dire;
        optional<address>                           addr;
        optional<int>                               skip;
        optional<int>                               take; 
    };
    void get_fungible_actions_async(int id, const get_fungible_actions_params& params);

    struct get_fungibles_balance_params {
        address addr;
    };
    void get_fungibles_balance_async(int id, const get_fungibles_balance_params& params);

    struct get_transaction_params {
        transaction_id_type id;
    };
    void get_transaction_async(int id, const get_transaction_params& params);

    struct get_transactions_params {
        std::vector<public_key_type>                keys;
        optional<fc::enum_type<uint8_t, direction>> dire;
        optional<int>                               skip;
        optional<int>                               take;
    };
    void get_transactions_async(int id, const get_transactions_params& params);

    struct get_fungible_ids_params {
        optional<int> skip;
        optional<int> take;
    };
    void get_fungible_ids_async(int id, const get_fungible_ids_params& params);

    using get_transaction_actions_params = get_transaction_params;
    void get_transaction_actions_async(int id, const get_transaction_actions_params& params);

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

}  // namespace jmzk

FC_REFLECT_ENUM(jmzk::history_apis::direction, (asc)(desc));
FC_REFLECT(jmzk::history_apis::read_only::get_params, (keys));
FC_REFLECT(jmzk::history_apis::read_only::get_tokens_params, (keys)(domain));
FC_REFLECT(jmzk::history_apis::read_only::get_actions_params, (domain)(key)(dire)(names)(skip)(take));
FC_REFLECT(jmzk::history_apis::read_only::get_fungible_actions_params, (sym_id)(dire)(addr)(skip)(take));
FC_REFLECT(jmzk::history_apis::read_only::get_fungibles_balance_params, (addr));
FC_REFLECT(jmzk::history_apis::read_only::get_transaction_params, (id));
FC_REFLECT(jmzk::history_apis::read_only::get_transactions_params, (keys)(dire)(skip)(take));
FC_REFLECT(jmzk::history_apis::read_only::get_fungible_ids_params, (skip)(take));
