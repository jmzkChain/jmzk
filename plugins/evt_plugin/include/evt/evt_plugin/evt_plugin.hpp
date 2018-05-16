/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <evt/chain_plugin/chain_plugin.hpp>

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
        group_id id;
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
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

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
FC_REFLECT(evt::evt_apis::read_only::get_group_params, (id));
FC_REFLECT(evt::evt_apis::read_only::get_token_params, (domain)(name));
FC_REFLECT(evt::evt_apis::read_only::get_account_params, (name));
