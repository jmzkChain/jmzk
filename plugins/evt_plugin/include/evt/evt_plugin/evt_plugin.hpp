/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain_plugin/chain_plugin.hpp>
#ifdef ENABLE_MONGODB
#include <evt/mongo_db_plugin/mongo_db_plugin.hpp>
#endif

#include <appbase/application.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/block.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/types.hpp>

namespace fc {
class variant;
}

namespace evt {

using namespace appbase;
using namespace evt::chain;
using namespace evt::chain::contracts;
using evt::chain_plugin;

class evt_plugin;

namespace evt_apis {

class read_only {
public:
    read_only(const controller& db)
        : db_(db) {}

public:
    struct get_domain_params {
        domain_name name;
    };

    fc::variant get_domain(const get_domain_params& params);

    struct get_group_params {
        group_name name;
    };

    fc::variant get_group(const get_group_params& params);

    struct get_token_params {
        domain_name domain;
        token_name  name;
    };

    fc::variant get_token(const get_token_params& params);

    struct get_account_params {
        account_name name;
    };
    fc::variant get_account(const get_account_params& params);

#ifdef ENABLE_MONGODB
    struct get_my_params {
        std::vector<std::string> signatures;
    };
    using get_my_tokens_params = get_my_params;
    using get_my_domains_params = get_my_params;
    using get_my_groups_params = get_my_params;

    fc::variant get_my_tokens(const get_my_params& params);
    fc::variant get_my_domains(const get_my_params& params);
    fc::variant get_my_groups(const get_my_params& params);
#endif

private:
    const controller& db_;
};

class read_write {
public:
    read_write(controller& db)
        : db_(db) {}

private:
    const controller& db_;
};

}  // namespace evt_apis

class evt_plugin : public plugin<evt_plugin> {
public:
#ifndef ENABLE_MONGODB
    APPBASE_PLUGIN_REQUIRES((chain_plugin))
#else
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(mongo_db_plugin))
#endif

    evt_plugin();
    virtual ~evt_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    evt_apis::read_only  get_read_only_api() const;
    evt_apis::read_write get_read_write_api();

private:
    std::unique_ptr<class evt_plugin_impl> my_;
};

}  // namespace evt

FC_REFLECT(evt::evt_apis::read_only::get_domain_params, (name));
FC_REFLECT(evt::evt_apis::read_only::get_group_params, (name));
FC_REFLECT(evt::evt_apis::read_only::get_token_params, (domain)(name));
FC_REFLECT(evt::evt_apis::read_only::get_account_params, (name));
#ifdef ENABLE_MONGODB
FC_REFLECT(evt::evt_apis::read_only::get_my_params, (signatures));
#endif