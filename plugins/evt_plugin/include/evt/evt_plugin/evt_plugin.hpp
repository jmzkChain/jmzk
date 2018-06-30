/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>

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

    struct get_fungible_params {
        fungible_name name;
    };
    fc::variant get_fungible(const get_fungible_params& params);

    struct get_assets_params {
        public_key_type      address;
        fc::optional<symbol> sym;
    };
    fc::variant get_assets(const get_assets_params& params);

    struct get_delay_params {
        proposal_name name;
    };
    fc::variant get_delay(const get_delay_params& params);

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
FC_REFLECT(evt::evt_apis::read_only::get_group_params, (name));
FC_REFLECT(evt::evt_apis::read_only::get_token_params, (domain)(name));
FC_REFLECT(evt::evt_apis::read_only::get_fungible_params, (name));
FC_REFLECT(evt::evt_apis::read_only::get_assets_params, (address)(sym));
FC_REFLECT(evt::evt_apis::read_only::get_delay_params, (name));
