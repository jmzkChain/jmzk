/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <chrono>
#include <functional>
#include <map>
#include <boost/signals2/signal.hpp>
#include <jmzk/chain/block_state.hpp>
#include <jmzk/chain/genesis_state.hpp>
#include <jmzk/chain/token_database.hpp>
#include <jmzk/chain/execution_context.hpp>

namespace chainbase {
class database;
}

namespace jmzk { namespace chain {

using unapplied_transactions_type = map<transaction_id_type, transaction_metadata_ptr>;

class fork_database;
class apply_context;
class charge_manager;
class execution_context;
class staking_context;
class token_database_cache;

struct controller_impl;
using boost::signals2::signal;
using chainbase::database;

class dynamic_global_property_object;
class global_property_object;

class snapshot_writer;
class snapshot_reader;

namespace contracts {
struct abi_serializer;
struct jmzk_link_object;
}  // namespace contracts

using contracts::abi_serializer;
using contracts::jmzk_link_object;

enum class db_read_mode {
    SPECULATIVE,
    HEAD,
    READ_ONLY,
    IRREVERSIBLE
};

enum class validation_mode {
    FULL,
    LIGHT
};

class controller {
public:
    struct config {
        path     blocks_dir             = chain::config::default_blocks_dir_name;
        path     state_dir              = chain::config::default_state_dir_name;
        uint64_t state_size             = chain::config::default_state_size;
        uint64_t state_guard_size       = chain::config::default_state_guard_size;
        uint64_t reversible_cache_size  = chain::config::default_reversible_cache_size;
        uint64_t reversible_guard_size  = chain::config::default_reversible_guard_size;
        bool     read_only              = false;
        bool     force_all_checks       = false;
        bool     disable_replay_opts    = false;
        bool     loadtest_mode          = false;
        bool     charge_free_mode       = false;
        bool     contracts_console      = false;

        std::chrono::microseconds max_serialization_time = std::chrono::milliseconds(chain::config::default_abi_serializer_max_time_ms);

        db_read_mode    read_mode             = db_read_mode::SPECULATIVE;
        validation_mode block_validation_mode = validation_mode::FULL;

        flat_set<account_name> trusted_producers;

        token_database::config db_config;

        genesis_state genesis;
    };

    enum class block_status {
        irreversible = 0, ///< this block has already been applied before by this node and is considered irreversible
        validated    = 1, ///< this is a complete block signed by a valid producer and has been previously applied by this node and therefore validated but it is not yet irreversible
        complete     = 2, ///< this is a complete block signed by a valid producer but is not yet irreversible nor has it yet been applied by this node
        incomplete   = 3, ///< this is an incomplete block (either being produced by a producer or speculatively produced by a node)
    };

    explicit controller(const config& cfg);
    ~controller();

    void add_indices();
    void startup(const std::shared_ptr<snapshot_reader>& snapshot = nullptr);

    /**
     * Starts a new pending block session upon which new transactions can
     * be pushed.
     */
    void start_block(block_timestamp_type time = block_timestamp_type(), uint16_t confirm_block_count = 0);

    void abort_block();

    /**
     *  These transactions were previously pushed by have since been unapplied, recalling push_transaction
     *  with the transaction_metadata_ptr will remove them from the source of this data IFF it succeeds.
     *
     *  The caller is responsible for calling drop_unapplied_transaction on a failing transaction that
     *  they never intend to retry
     *
     *  @return map of transactions which have been unapplied
     */
    unapplied_transactions_type& get_unapplied_transactions() const;

    transaction_trace_ptr push_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline);
    transaction_trace_ptr push_suspend_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline);

    void check_authorization(const public_keys_set& signed_keys, const transaction& trx);
    void check_authorization(const public_keys_set& signed_keys, const action& act);

    void finalize_block();
    void sign_block(const std::function<signature_type(const digest_type&)>& signer_callback);
    void commit_block();
    void pop_block();

    void push_block(const signed_block_ptr& b);

    chainbase::database& db() const;
    fork_database& fork_db() const;
    token_database& token_db() const;
    token_database_cache& token_db_cache() const;

    charge_manager get_charge_manager() const;

    execution_context& get_execution_context() const;

    const global_property_object&         get_global_properties() const;
    const dynamic_global_property_object& get_dynamic_global_properties() const;

    uint32_t            head_block_num() const;
    time_point          head_block_time() const;
    block_id_type       head_block_id() const;
    account_name        head_block_producer() const;
    const block_header& head_block_header() const;
    block_state_ptr     head_block_state() const;

    uint32_t      fork_db_head_block_num() const;
    block_id_type fork_db_head_block_id() const;
    time_point    fork_db_head_block_time() const;
    account_name  fork_db_head_block_producer()const;

    time_point              pending_block_time() const;
    block_state_ptr         pending_block_state() const;
    optional<block_id_type> pending_producer_block_id() const;

    const producer_schedule_type&    active_producers() const;
    const producer_schedule_type&    pending_producers() const;
    optional<producer_schedule_type> proposed_producers() const;

    uint32_t      last_irreversible_block_num() const;
    block_id_type last_irreversible_block_id() const;

    signed_block_ptr fetch_block_by_number(uint32_t block_num) const;
    signed_block_ptr fetch_block_by_id(block_id_type id) const;

    block_state_ptr fetch_block_state_by_number(uint32_t block_num) const;
    block_state_ptr fetch_block_state_by_id(block_id_type id) const;

    block_id_type   get_block_id_for_num(uint32_t block_num) const;
    jmzk_link_object get_link_obj_for_link_id(const link_id_type&) const;
    uint32_t        get_block_num_for_trx_id(const transaction_id_type& trx_id) const;

    fc::sha256 calculate_integrity_hash() const;
    void write_snapshot(const std::shared_ptr<snapshot_writer>& snapshot) const;

    bool is_producing_block() const;

    void validate_expiration(const transaction& t) const;
    void validate_tapos(const transaction& t) const;
    void validate_db_available_size() const;
    void validate_reversible_available_size() const;

    bool is_known_unexpired_transaction(const transaction_id_type& id) const;

    int64_t set_proposed_producers(vector<producer_key> producers);
    void    set_chain_config(const chain_config&);
    void    set_action_versions(vector<action_ver> vers);
    void    set_action_version(name action, int version);
    void    set_initial_staking_period();

    bool light_validation_allowed(bool replay_opts_disabled_by_policy) const;
    bool skip_auth_check() const;
    bool skip_db_sessions() const;
    bool skip_db_sessions(block_status bs) const;
    bool skip_trx_checks() const;
    bool loadtest_mode() const;
    bool charge_free_mode() const;
    bool contracts_console() const;

    db_read_mode    get_read_mode() const;
    validation_mode get_validation_mode() const;

    const chain_id_type& get_chain_id() const;
    const genesis_state& get_genesis_state() const;

    signal<void(const signed_block_ptr&)>         pre_accepted_block;
    signal<void(const block_state_ptr&)>          accepted_block_header;
    signal<void(const block_state_ptr&)>          accepted_block;
    signal<void(const block_state_ptr&)>          irreversible_block;
    signal<void(const transaction_metadata_ptr&)> accepted_transaction;
    signal<void(const transaction_trace_ptr&)>    applied_transaction;
    signal<void(const int&)>                      bad_alloc;

    public_keys_set get_required_keys(const transaction& trx, const public_keys_set& candidate_keys) const;
    public_keys_set get_suspend_required_keys(const transaction& trx, const public_keys_set& candidate_keys) const;
    public_keys_set get_suspend_required_keys(const proposal_name& name, const public_keys_set& candidate_keys) const;
    public_keys_set get_jmzklink_signed_keys(const link_id_type& link_id) const;

    uint32_t get_charge(transaction&& trx, size_t signautres_num) const;

    const abi_serializer& get_abi_serializer() const;

private:
    std::unique_ptr<controller_impl> my;
};

}}  // namespace jmzk::chain

FC_REFLECT(jmzk::chain::controller::config,
           (blocks_dir)
           (state_dir)
           (state_size)
           (reversible_cache_size)
           (read_only)
           (force_all_checks)
           (disable_replay_opts)
           (loadtest_mode)
           (charge_free_mode)
           (contracts_console)
           (trusted_producers)
           (db_config)
           (genesis)
           );
