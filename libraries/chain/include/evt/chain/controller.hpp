/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <functional>
#include <boost/signals2/signal.hpp>
#include <evt/chain/block_state.hpp>
#include <evt/chain/trace.hpp>
#include <evt/chain/genesis_state.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>

namespace chainbase {
class database;
}

namespace evt { namespace chain {

class fork_database;
class token_database;
class apply_context;
class charge_manager;

struct controller_impl;
using boost::signals2::signal;
using chainbase::database;
using contracts::abi_serializer;

class dynamic_global_property_object;
class global_property_object;

class controller {
public:
    struct config {
        path     blocks_dir             = chain::config::default_blocks_dir_name;
        path     state_dir              = chain::config::default_state_dir_name;
        path     tokendb_dir            = chain::config::default_tokendb_dir_name;
        uint64_t state_size             = chain::config::default_state_size;
        uint64_t state_guard_size       = chain::config::default_state_guard_size;
        uint64_t reversible_cache_size  = chain::config::default_reversible_cache_size;
        uint64_t reversible_guard_size  = chain::config::default_reversible_guard_size;
        bool     read_only              = false;
        bool     force_all_checks       = false;
        bool     loadtest_mode          = false;
        bool     charge_free_mode       = false;
        bool     contracts_console      = false;

        genesis_state genesis;
    };

    enum class block_status {
        irreversible = 0, ///< this block has already been applied before by this node and is considered irreversible
        validated    = 1, ///< this is a complete block signed by a valid producer and has been previously applied by this node and therefore validated but it is not yet irreversible
        complete     = 2, ///< this is a complete block signed by a valid producer but is not yet irreversible nor has it yet been applied by this node
        incomplete   = 3, ///< this is an incomplete block (either being produced by a producer or speculatively produced by a node)
    };

    controller(const config& cfg);
    ~controller();

    void startup();

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
          *  @return vector of transactions which have been unapplied
          */
    vector<transaction_metadata_ptr> get_unapplied_transactions() const;
    void                             drop_unapplied_transaction(const transaction_metadata_ptr& trx);

    transaction_trace_ptr push_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline);
    transaction_trace_ptr push_suspend_transaction(const transaction_metadata_ptr& trx, fc::time_point deadline);

    void check_authorization(const flat_set<public_key_type>& signed_keys, const transaction& trx);
    void check_authorization(const flat_set<public_key_type>& signed_keys, const action& act);

    void finalize_block();
    void sign_block(const std::function<signature_type(const digest_type&)>& signer_callback);
    void commit_block();
    void pop_block();

    void push_block(const signed_block_ptr& b, block_status s = block_status::complete);

    /**
          * Call this method when a producer confirmation is received, this might update
          * the last bft irreversible block and/or cause a switch of forks
          */
    void push_confirmation(const header_confirmation& c);

    chainbase::database& db() const;
    fork_database& fork_db() const;
    token_database& token_db() const;

    charge_manager get_charge_manager() const;

    const global_property_object&         get_global_properties() const;
    const dynamic_global_property_object& get_dynamic_global_properties() const;

    uint32_t            head_block_num() const;
    time_point          head_block_time() const;
    block_id_type       head_block_id() const;
    account_name        head_block_producer() const;
    const block_header& head_block_header() const;
    block_state_ptr     head_block_state() const;

    uint32_t      fork_db_head_block_num()const;
    block_id_type fork_db_head_block_id()const;
    time_point    fork_db_head_block_time()const;
    account_name  fork_db_head_block_producer()const;

    time_point      pending_block_time() const;
    block_state_ptr pending_block_state() const;

    const producer_schedule_type&    active_producers() const;
    const producer_schedule_type&    pending_producers() const;
    optional<producer_schedule_type> proposed_producers() const;

    uint32_t      last_irreversible_block_num() const;
    block_id_type last_irreversible_block_id() const;

    signed_block_ptr fetch_block_by_number(uint32_t block_num) const;
    signed_block_ptr fetch_block_by_id(block_id_type id) const;

    block_state_ptr fetch_block_state_by_number(uint32_t block_num) const;
    block_state_ptr fetch_block_state_by_id(block_id_type id) const;

    block_id_type get_block_id_for_num(uint32_t block_num) const;
    transaction_id_type get_trx_id_for_link_id(const link_id_type&) const;

    bool is_producing_block() const;

    void validate_expiration(const transaction& t) const;
    void validate_tapos(const transaction& t) const;
    void validate_db_available_size() const;
    void validate_reversible_available_size() const;

    bool is_known_unexpired_transaction(const transaction_id_type& id) const;

    int64_t set_proposed_producers(vector<producer_key> producers);
    void    set_chain_config(const chain_config&);

    bool skip_auth_check() const;
    bool loadtest_mode() const;
    bool charge_free_mode() const;
    bool contracts_console() const;

    chain_id_type get_chain_id() const;

    signal<void(const signed_block_ptr&)>         pre_accepted_block;
    signal<void(const block_state_ptr&)>          accepted_block_header;
    signal<void(const block_state_ptr&)>          accepted_block;
    signal<void(const block_state_ptr&)>          irreversible_block;
    signal<void(const transaction_metadata_ptr&)> accepted_transaction;
    signal<void(const transaction_trace_ptr&)>    applied_transaction;
    signal<void(const header_confirmation&)>      accepted_confirmation;
    signal<void(const int&)>                      bad_alloc;

    flat_set<public_key_type> get_required_keys(const transaction& trx, const flat_set<public_key_type>& candidate_keys) const;
    flat_set<public_key_type> get_suspend_required_keys(const transaction& trx, const flat_set<public_key_type>& candidate_keys) const;
    flat_set<public_key_type> get_suspend_required_keys(const proposal_name& name, const flat_set<public_key_type>& candidate_keys) const;

    uint32_t get_charge(const transaction& trx, size_t signautres_num) const;

    const abi_serializer& get_abi_serializer() const;

    template <typename T>
    fc::variant
    to_variant_with_abi(const T& obj) {
        fc::variant pretty_output;
        abi_serializer::to_variant(obj, pretty_output, [this]() -> const abi_serializer& { return get_abi_serializer(); });
        return pretty_output;
    }

private:
    std::unique_ptr<controller_impl> my;
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::controller::config,
           (blocks_dir)(state_dir)(tokendb_dir)(state_size)(reversible_cache_size)(read_only)(force_all_checks)(loadtest_mode)(charge_free_mode)(contracts_console)(genesis))
