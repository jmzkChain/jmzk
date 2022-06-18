/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <jmzk/chain/asset.hpp>
#include <jmzk/chain/block.hpp>
#include <jmzk/chain/version.hpp>
#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/transaction.hpp>
#include <jmzk/chain/staking_context.hpp>
#include <jmzk/chain/plugin_interface.hpp>
#include <jmzk/chain/percent_slim.hpp>
#include <jmzk/chain/contracts/abi_serializer.hpp>

#include <fc/static_variant.hpp>

#include <boost/container/flat_set.hpp>

namespace fc {
class variant;
}

namespace jmzk {
using namespace appbase;
using std::unique_ptr;
using std::optional;

using boost::container::flat_set;

using chain::account_name;
using chain::bytes;
using chain::block_id_type;
using chain::chain_id_type;
using chain::controller;
using chain::digest_type;
using chain::name;
using chain::proposal_name;
using chain::public_key_type;
using chain::public_keys_set;
using chain::transaction_id_type;
using chain::version;
using chain::asset;
using chain::contracts::abi_serializer;
using chain::contracts::validator_def;
using chain::percent_slim;

namespace chain_apis {
struct empty {};

template <typename>
struct resolver_factory;

class read_only {
public:
    const controller& db;
    bool  shorten_abi_errors = true;

public:
    read_only(const controller& db)
        : db(db) {}

    void set_shorten_abi_errors(bool f) { shorten_abi_errors = f; }

public:
    using get_info_params = empty;

    struct get_info_results {
        string             server_version;
        chain_id_type      chain_id;
        version            jmzk_api_version;
        uint32_t           head_block_num              = 0;
        uint32_t           last_irreversible_block_num = 0;
        block_id_type      last_irreversible_block_id;
        block_id_type      head_block_id;
        fc::time_point_sec head_block_time;
        account_name       head_block_producer;
        vector<string>     enabled_plugins;
        optional<string>   server_version_string;
    };
    get_info_results get_info(const get_info_params&) const;

    using get_charge_info_params = empty;

    struct get_charge_info_results {
        uint32_t base_network_charge_factor;
        uint32_t base_storage_charge_factor;
        uint32_t base_cpu_charge_factor;
        uint32_t global_charge_factor;
    };
    get_charge_info_results get_charge_info(const get_charge_info_params&) const;

    struct producer_info {
        name producer_name;
    };

    struct abi_json_to_bin_params {
        name        action;
        fc::variant args;
    };
    struct abi_json_to_bin_result {
        vector<char> binargs;
    };
    abi_json_to_bin_result abi_json_to_bin(const abi_json_to_bin_params& params) const;

    struct abi_bin_to_json_params {
        name         action;
        vector<char> binargs;
    };
    struct abi_bin_to_json_result {
        fc::variant args;
    };
    abi_bin_to_json_result abi_bin_to_json(const abi_bin_to_json_params& params) const;

    using trx_json_to_digest_params = fc::variant_object;
    struct trx_json_to_digest_result {
        digest_type         digest;
        transaction_id_type id;
    };
    trx_json_to_digest_result trx_json_to_digest(const trx_json_to_digest_params& params) const;

    using trx_json_to_bin_params = fc::variant_object;
    using trx_json_to_bin_result = abi_json_to_bin_result;
    trx_json_to_bin_result trx_json_to_bin(const trx_json_to_bin_params& params) const;

    struct get_required_keys_params {
        fc::variant      transaction;
        public_keys_set  available_keys;
    };
    struct get_required_keys_result {
        public_keys_set  required_keys;
    };
    get_required_keys_result get_required_keys(const get_required_keys_params& params) const;

    struct get_suspend_required_keys_params {
        proposal_name             name;
        public_keys_set  available_keys;
    };
    struct get_suspend_required_keys_result {
        public_keys_set  required_keys;
    };
    get_suspend_required_keys_result get_suspend_required_keys(const get_suspend_required_keys_params& params) const;

    struct get_charge_params {
        fc::variant transaction;
        int         sigs_num;      
    };
    struct get_charge_result {
        uint32_t    charge;
    };
    get_charge_result get_charge(const get_charge_params& params) const;

    struct get_block_params {
        string block_num_or_id;
    };
    fc::variant get_block(const get_block_params& params) const;

    struct get_block_header_state_params {
        string block_num_or_id;
    };
    fc::variant get_block_header_state(const get_block_header_state_params& params) const;

    using get_head_block_header_state_params = empty;
    fc::variant get_head_block_header_state(const get_head_block_header_state_params& params) const;

    struct get_transaction_params {
        optional<uint32_t>  block_num;
        transaction_id_type id;
        optional<bool>      raw;
    };
    fc::variant get_transaction(const get_transaction_params& params);

    struct get_trx_id_for_link_id_params {
        bytes link_id;
    };
    fc::variant get_trx_id_for_link_id(const get_trx_id_for_link_id_params& params) const;

    struct get_transaction_ids_for_block_params {
        block_id_type block_id;
    };
    fc::variant get_transaction_ids_for_block(const get_transaction_ids_for_block_params& params) const;

    using get_abi_params = empty;
    const std::string& get_abi(const get_abi_params&) const;

    using get_actions_params = empty;
    const std::string& get_actions(const get_actions_params&) const;

    using get_staking_params = empty;
    struct validator_slim {
        account_name name;
        asset        current_net_value;
        int64_t      total_units;
        percent_slim commission;
    };
    struct get_staking_result {
        uint32_t period_version ; 
        uint32_t period_start_num;
        uint32_t next_period_num;
        std::vector<validator_slim> validators;
    };
    get_staking_result get_staking(const get_staking_params& params) const;

    using get_db_info_params = empty;
    std::string get_db_info(const get_db_info_params&) const;
};

class read_write {
public:
    controller& db;

public:
    read_write(controller& db)
        : db(db) {}

    using push_block_params  = chain::signed_block;
    using push_block_results = empty;
    void push_block(push_block_params&& params, chain::plugin_interface::next_function<push_block_results> next);

    using push_transaction_params = fc::variant_object;
    struct push_transaction_results {
        transaction_id_type transaction_id;
        fc::variant         processed;
    };
    void push_transaction(const push_transaction_params& params, chain::plugin_interface::next_function<push_transaction_results> next);

    using push_transactions_params  = vector<push_transaction_params>;
    using push_transactions_results = vector<push_transaction_results>;
    void push_transactions(const push_transactions_params& params, chain::plugin_interface::next_function<push_transactions_results> next);

    friend resolver_factory<read_write>;
};
}  // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES()

    chain_plugin();
    virtual ~chain_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    chain_apis::read_only  get_read_only_api() const;
    chain_apis::read_write get_read_write_api();

    void accept_block(const chain::signed_block_ptr& block );
    void accept_transaction(const chain::packed_transaction& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);
    void accept_transaction(const chain::transaction_metadata_ptr& trx, chain::plugin_interface::next_function<chain::transaction_trace_ptr> next);

    bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

    static bool recover_reversible_blocks(const fc::path& db_dir, uint32_t cache_size, optional<fc::path> new_db_dir = optional<fc::path>(), uint32_t truncate_at_block = 0);

    static bool import_reversible_blocks(const fc::path& reversible_dir, uint32_t cache_size, const fc::path& reversible_blocks_file);

    static bool export_reversible_blocks(const fc::path& reversible_dir, const fc::path& reversible_blocks_file);
    // return true if --skip-transaction-signatures passed to jmzkd
    bool is_skipping_transaction_signatures() const;

    // Only call this after plugin_initialize()!
    controller& chain();

    // Only call this after plugin_startup()!
    const controller& chain() const;

    chain::chain_id_type get_chain_id() const;

    void handle_guard_exception(const chain::guard_exception& e) const;

    static void handle_db_exhaustion();

private:
    void log_guard_exception(const chain::guard_exception& e) const;

    unique_ptr<class chain_plugin_impl> my;
};

}  // namespace jmzk

FC_REFLECT(jmzk::chain_apis::empty, );
FC_REFLECT(jmzk::chain_apis::read_only::get_info_results,
          (server_version)(chain_id)(jmzk_api_version)(head_block_num)(last_irreversible_block_num)(last_irreversible_block_id)
          (head_block_id)(head_block_time)(head_block_producer)(enabled_plugins)(server_version_string));
FC_REFLECT(jmzk::chain_apis::read_only::get_charge_info_results, (base_network_charge_factor)(base_storage_charge_factor)(base_cpu_charge_factor)(global_charge_factor));
FC_REFLECT(jmzk::chain_apis::read_only::get_block_params, (block_num_or_id));
FC_REFLECT(jmzk::chain_apis::read_only::get_block_header_state_params, (block_num_or_id));
FC_REFLECT(jmzk::chain_apis::read_only::get_transaction_params, (block_num)(id));
FC_REFLECT(jmzk::chain_apis::read_only::get_trx_id_for_link_id_params, (link_id));
FC_REFLECT(jmzk::chain_apis::read_only::producer_info, (producer_name));
FC_REFLECT(jmzk::chain_apis::read_only::abi_json_to_bin_params, (action)(args));
FC_REFLECT(jmzk::chain_apis::read_only::abi_json_to_bin_result, (binargs));
FC_REFLECT(jmzk::chain_apis::read_only::abi_bin_to_json_params, (action)(binargs));
FC_REFLECT(jmzk::chain_apis::read_only::abi_bin_to_json_result, (args));
FC_REFLECT(jmzk::chain_apis::read_only::trx_json_to_digest_result, (digest)(id));
FC_REFLECT(jmzk::chain_apis::read_only::get_required_keys_params, (transaction)(available_keys));
FC_REFLECT(jmzk::chain_apis::read_only::get_required_keys_result, (required_keys));
FC_REFLECT(jmzk::chain_apis::read_only::get_suspend_required_keys_params, (name)(available_keys));
FC_REFLECT(jmzk::chain_apis::read_only::get_suspend_required_keys_result, (required_keys));
FC_REFLECT(jmzk::chain_apis::read_only::get_charge_params, (transaction)(sigs_num));
FC_REFLECT(jmzk::chain_apis::read_only::get_charge_result, (charge));
FC_REFLECT(jmzk::chain_apis::read_only::validator_slim, (name)(current_net_value)(total_units)(commission));
FC_REFLECT(jmzk::chain_apis::read_only::get_staking_result, (period_version)(period_start_num)(next_period_num)(validators));
FC_REFLECT(jmzk::chain_apis::read_only::get_transaction_ids_for_block_params, (block_id));
FC_REFLECT(jmzk::chain_apis::read_write::push_transaction_results, (transaction_id)(processed));
