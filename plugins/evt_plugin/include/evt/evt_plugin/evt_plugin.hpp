/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/contracts/types.hpp>

namespace fc { class variant; }

namespace eosio {
using chain::chain_controller;
using std::unique_ptr;
using namespace appbase;
using chain::name;
using chain::uint128_t;
using chain::public_key_type;
using fc::optional;
using boost::container::flat_set;
using chain::asset;
using chain::authority;
using chain::account_name;
using chain::domain_name;
using chain::domain_key;
using chain::token_name;
using chain::contracts::group_id;
using chain::contracts::abi_def;
using chain::contracts::abi_serializer;
using chain::contracts::domain_def;
using chain::contracts::token_def;
using chain::contracts::group_def;
using chain::contracts::newdomain;
using chain::contracts::issuetoken;
using chain::contracts::transfer;
using chain::contracts::updatedomain;
using chain::contracts::updategroup;

namespace evt_apis {

class read_only {
public:
    read_only(const chain_controller& db) : db_(db) {}

public:
    struct get_domain_params {
        domain_name name;
    };

    fc::variant get_domain(const get_domain_params& params);

    struct get_group_params {
        group_id    id;
    };

    fc::variant get_group(const get_group_params& params);

    struct get_token_params {
        domain_name domain;
        token_name  name;
    };

    fc::variant get_token(const get_token_params& params);


private:
    const chain_controller& db_;

};

class read_write {
public:
    read_write(chain_controller& db) : db_(db) {}

public:
    struct new_domain_result {
        domain_name     name;
    };
    new_domain_result new_domain(const newdomain& params);

    struct issue_tokens_result {
        domain_name             domain;
        std::vector<token_name> names;
    };
    issue_tokens_result issue_tokens(const issuetoken& params);

    struct transfer_result {
        token_def   token;
    };
    transfer_result transfer_token(const transfer& params);

private:
    const chain_controller& db_;

};

} // namespace evt_apis

class evt_plugin : public plugin<evt_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    evt_plugin();
    virtual ~evt_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    evt_apis::read_only get_read_only_api() const;
    evt_apis::read_write get_read_write_api();

private:
    std::unique_ptr<class evt_plugin_impl> my_;
};

}  // namespace eosio

FC_REFLECT(eosio::evt_apis::read_only::get_domain_params, (name));
FC_REFLECT(eosio::evt_apis::read_only::get_group_params, (id));
FC_REFLECT(eosio::evt_apis::read_only::get_token_params, (domain)(name));
FC_REFLECT(eosio::evt_apis::read_write::new_domain_result, (name));
FC_REFLECT(eosio::evt_apis::read_write::issue_tokens_result, (domain)(names));
FC_REFLECT(eosio::evt_apis::read_write::transfer_result, (token));
