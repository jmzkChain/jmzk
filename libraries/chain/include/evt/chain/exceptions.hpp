/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <boost/core/typeinfo.hpp>
#include <fc/exception/exception.hpp>

#define EVT_ASSERT(expr, exc_type, FORMAT, ...)            \
    FC_MULTILINE_MACRO_BEGIN                               \
    if(!(expr))                                            \
        FC_THROW_EXCEPTION(exc_type, FORMAT, __VA_ARGS__); \
    FC_MULTILINE_MACRO_END

#define EVT_THROW(exc_type, FORMAT, ...) throw exc_type(FC_LOG_MESSAGE(error, FORMAT, __VA_ARGS__));

/**
 * Macro inspired from FC_RETHROW_EXCEPTIONS
 * The main difference here is that if the exception caught isn't of type "chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EVT_RETHROW_EXCEPTIONS(exception_type, FORMAT, ...)                                                 \
    catch(const boost::interprocess::bad_alloc&) {                                                          \
        throw;                                                                                              \
    }                                                                                                       \
    catch(const fc::unrecoverable_exception&) {                                                             \
        throw;                                                                                              \
    }                                                                                                       \
    catch(chain_exception & e) {                                                                            \
        FC_RETHROW_EXCEPTION(e, warn, FORMAT, __VA_ARGS__);                                                 \
    }                                                                                                       \
    catch(fc::exception & e) {                                                                              \
        exception_type new_exception(FC_LOG_MESSAGE(warn, FORMAT, __VA_ARGS__));                            \
        for(const auto& log : e.get_log()) {                                                                \
            new_exception.append_log(log);                                                                  \
        }                                                                                                   \
        throw new_exception;                                                                                \
    }                                                                                                       \
    catch(const std::exception& e) {                                                                        \
        exception_type fce(FC_LOG_MESSAGE(warn, FORMAT " (${what})", __VA_ARGS__("what", e.what())));       \
        throw fce;                                                                                          \
    }                                                                                                       \
    catch(...) {                                                                                            \
        throw fc::unhandled_exception(FC_LOG_MESSAGE(warn, FORMAT, __VA_ARGS__), std::current_exception()); \
    }

/**
 * Macro inspired from FC_CAPTURE_AND_RETHROW
 * The main difference here is that if the exception caught isn't of type "chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EVT_CAPTURE_AND_RETHROW(exception_type, ...)                                                               \
    catch(const boost::interprocess::bad_alloc&) {                                                                 \
        throw;                                                                                                     \
    }                                                                                                              \
    catch(const fc::unrecoverable_exception&) {                                                                    \
        throw;                                                                                                     \
    }                                                                                                              \
    catch(chain_exception & e) {                                                                                   \
        FC_RETHROW_EXCEPTION(e, warn, "", FC_FORMAT_ARG_PARAMS(__VA_ARGS__));                                      \
    }                                                                                                              \
    catch(fc::exception & e) {                                                                                     \
        exception_type new_exception(e.get_log());                                                                 \
        throw new_exception;                                                                                       \
    }                                                                                                              \
    catch(const std::exception& e) {                                                                               \
        exception_type fce(FC_LOG_MESSAGE(warn, "${what}: ", FC_FORMAT_ARG_PARAMS(__VA_ARGS__)("what", e.what())), \
                           fc::std_exception_code, BOOST_CORE_TYPEID(decltype(e)).name(), e.what());               \
        throw fce;                                                                                                 \
    }                                                                                                              \
    catch(...) {                                                                                                   \
        throw fc::unhandled_exception(FC_LOG_MESSAGE(warn, "", FC_FORMAT_ARG_PARAMS(__VA_ARGS__)),                 \
                                      std::current_exception());                                                   \
    }

#define EVT_RECODE_EXC(cause_type, effect_type)    \
    catch(const cause_type& e) {                   \
        throw(effect_type(e.what(), e.get_log())); \
    }

namespace evt { namespace chain {

FC_DECLARE_EXCEPTION( chain_exception, 3000000, "blockchain exception" );
FC_DECLARE_DERIVED_EXCEPTION( database_exception,                chain_exception, 3010000, "Database exception" );
FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          chain_exception, 3020000, "block validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception, 3030000, "transaction validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( action_exception,                  chain_exception, 3040000, "action validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( producer_exception,                chain_exception, 3050000, "Producer exception" );
FC_DECLARE_DERIVED_EXCEPTION( block_log_exception,               chain_exception, 3060000, "Block log exception" );
FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 chain_exception, 3070000, "utility method exception" );
FC_DECLARE_DERIVED_EXCEPTION( fork_database_exception,           chain_exception, 3080000, "Fork database exception" );
FC_DECLARE_DERIVED_EXCEPTION( reversible_blocks_exception,       chain_exception, 3090000, "Reversible Blocks exception" );
FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              chain_exception, 3100000, "black swan" );
FC_DECLARE_DERIVED_EXCEPTION( chain_type_exception,              chain_exception, 3110000, "chain type exception" );
FC_DECLARE_DERIVED_EXCEPTION( plugin_exception,                  chain_exception, 3120000, "plugin exception" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_exception,                  chain_exception, 3130000, "wallet exception" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_exception,                 chain_exception, 3140000, "tokendb exception" );
FC_DECLARE_DERIVED_EXCEPTION( misc_exception,                    chain_exception, 3150000, "Miscellaneous exception");
FC_DECLARE_DERIVED_EXCEPTION( authorization_exception,           chain_exception, 3160000, "Authorization exception");
FC_DECLARE_DERIVED_EXCEPTION( controller_emit_signal_exception,  chain_exception, 3170000, "Exceptions that are allowed to bubble out of emit calls in controller" );
FC_DECLARE_DERIVED_EXCEPTION( http_exception,                    chain_exception, 3180000, "http exception" );

FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        block_validate_exception, 3020001, "Unlinkable block" );
FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         block_validate_exception, 3020002, "Transaction outputs in block do not match transaction outputs from applying block" );
FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception,       block_validate_exception, 3020003, "Block does not guarantee concurrent execution without conflicts" );
FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,              block_validate_exception, 3020004, "Shard locks in block are incorrect or mal-formed" );
FC_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,          block_validate_exception, 3020005, "Block exhausted allowed resources" );
FC_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,           block_validate_exception, 3020006, "Block is too old to push" );
FC_DECLARE_DERIVED_EXCEPTION( block_from_the_future,             block_validate_exception, 3020007, "Block is from the future" );
FC_DECLARE_DERIVED_EXCEPTION( wrong_signing_key,                 block_validate_exception, 3020008, "Block is not signed with expected key" );
FC_DECLARE_DERIVED_EXCEPTION( wrong_producer,                    block_validate_exception, 3020009, "Block is not signed by expected producer" );

FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      transaction_exception, 3030001, "duplicate transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,            transaction_exception, 3030002, "Error decompressing transaction" );
FC_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,              transaction_exception, 3030003, "Expired Transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,          transaction_exception, 3030004, "Transaction Expiration Too Far" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception,       transaction_exception, 3030005, "Invalid Reference Block" );
FC_DECLARE_DERIVED_EXCEPTION( tx_apply_exception,                transaction_exception, 3030006, "Transaction Apply Exception" );
FC_DECLARE_DERIVED_EXCEPTION( tx_receipt_inconsistent_status,    transaction_exception, 3030007, "Transaction receipt applied status does not match received status." );
FC_DECLARE_DERIVED_EXCEPTION( tx_no_action,                      transaction_exception, 3030008, "transaction should have at least one normal action." );
FC_DECLARE_DERIVED_EXCEPTION( deadline_exception,                transaction_exception, 3030009, "transaction is timeout." );
FC_DECLARE_DERIVED_EXCEPTION( max_charge_exceeded_exception,     transaction_exception, 3030010, "exceeded max charge paid");
FC_DECLARE_DERIVED_EXCEPTION( charge_exceeded_exception,         transaction_exception, 3030011, "exceeded remaining EVT & Pinned EVT tokens" );
FC_DECLARE_DERIVED_EXCEPTION( payer_exception,                   transaction_exception, 3030012, "Invalid payer" );
FC_DECLARE_DERIVED_EXCEPTION( too_many_tx_at_once,               transaction_exception, 3040013, "Pushing too many transactions at once" );
FC_DECLARE_DERIVED_EXCEPTION( tx_too_big,                        transaction_exception, 3040014, "Transaction is too big" );
FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_compression,   transaction_exception, 3040015, "Unknown transaction compression" );

FC_DECLARE_DERIVED_EXCEPTION( producer_priv_key_not_found,       producer_exception, 3050001, "Producer private key is not available" );
FC_DECLARE_DERIVED_EXCEPTION( missing_pending_block_state,       producer_exception, 3050002, "Pending block state is missing" );
FC_DECLARE_DERIVED_EXCEPTION( producer_double_confirm,           producer_exception, 3050003, "Producer is double confirming known range" );
FC_DECLARE_DERIVED_EXCEPTION( producer_schedule_exception,       producer_exception, 3050004, "Producer schedule exception" );
FC_DECLARE_DERIVED_EXCEPTION( producer_not_in_schedule,          producer_exception, 3050006, "The producer is not part of current schedule" );
   
FC_DECLARE_DERIVED_EXCEPTION( block_log_unsupported_version,     block_log_exception, 3060001, "unsupported version of block log" );
FC_DECLARE_DERIVED_EXCEPTION( block_log_append_fail,             block_log_exception, 3060002, "fail to append block to the block log" );
FC_DECLARE_DERIVED_EXCEPTION( block_log_not_found,               block_log_exception, 3060003, "block log can not be found" );
FC_DECLARE_DERIVED_EXCEPTION( block_log_backup_dir_exist,        block_log_exception, 3060004, "block log backup dir already exists" );

FC_DECLARE_DERIVED_EXCEPTION( fork_db_block_not_found,           fork_database_exception, 3080001, "Block can not be found" );

FC_DECLARE_DERIVED_EXCEPTION( invalid_reversible_blocks_dir,      reversible_blocks_exception, 3090001, "Invalid reversible blocks directory" );
FC_DECLARE_DERIVED_EXCEPTION( reversible_blocks_backup_dir_exist, reversible_blocks_exception, 3090002, "Backup directory for reversible blocks already existg" );
FC_DECLARE_DERIVED_EXCEPTION( gap_in_reversible_blocks_db,        reversible_blocks_exception, 3090003, "Gap in the reversible blocks database" );

FC_DECLARE_DERIVED_EXCEPTION( guard_exception,                    database_exception, 3160101, "Database exception" );
FC_DECLARE_DERIVED_EXCEPTION( database_guard_exception,           guard_exception,    3160102, "Database usage is at unsafe levels" );
FC_DECLARE_DERIVED_EXCEPTION( reversible_guard_exception,         guard_exception,    3160103, "Reversible block log usage is at unsafe levels" );

FC_DECLARE_DERIVED_EXCEPTION( checkpoint_exception,               controller_emit_signal_exception, 3170001, "Block does not match checkpoint" );

FC_DECLARE_DERIVED_EXCEPTION( action_authorize_exception,          action_exception, 3040001, "invalid action authorization" );
FC_DECLARE_DERIVED_EXCEPTION( action_args_exception,               action_exception, 3040002, "Invalid arguments for action" );
FC_DECLARE_DERIVED_EXCEPTION( domain_not_existed_exception,        action_exception, 3040003, "Domain does not exist." );
FC_DECLARE_DERIVED_EXCEPTION( token_not_existed_exception,         action_exception, 3040004, "Token does not exist." );
FC_DECLARE_DERIVED_EXCEPTION( group_not_existed_exception,         action_exception, 3040005, "Group does not exist." );
FC_DECLARE_DERIVED_EXCEPTION( fungible_not_existed_exception,      action_exception, 3040006, "Fungible asset does not exist." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_not_existed_exception,       action_exception, 3040007, "Suspend transaction does not exist." );
FC_DECLARE_DERIVED_EXCEPTION( domain_exists_exception,             action_exception, 3040008, "Domain already exists." );
FC_DECLARE_DERIVED_EXCEPTION( token_exists_exception,              action_exception, 3040009, "Token already exists." );
FC_DECLARE_DERIVED_EXCEPTION( group_exists_exception,              action_exception, 3040010, "Group already exists." );
FC_DECLARE_DERIVED_EXCEPTION( fungible_exists_exception,           action_exception, 3040011, "Fungible asset already exists." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_exists_exception,            action_exception, 3040012, "Suspend transaction already exists." );
FC_DECLARE_DERIVED_EXCEPTION( domain_name_exception,               action_exception, 3040013, "Invalid domain name" );
FC_DECLARE_DERIVED_EXCEPTION( token_name_exception,                action_exception, 3040014, "Invalid token name" );
FC_DECLARE_DERIVED_EXCEPTION( group_name_exception,                action_exception, 3040015, "Invalid group name" );
FC_DECLARE_DERIVED_EXCEPTION( group_key_exception,                 action_exception, 3040016, "Group key is reserved to update this group." );
FC_DECLARE_DERIVED_EXCEPTION( meta_key_exception,                  action_exception, 3040017, "Invalid meta key" );
FC_DECLARE_DERIVED_EXCEPTION( proposal_name_exception,             action_exception, 3040018, "Invalid proposal name" );
FC_DECLARE_DERIVED_EXCEPTION( fungible_name_exception,             action_exception, 3040019, "Invalid fungible asset name" );
FC_DECLARE_DERIVED_EXCEPTION( fungible_symbol_exception,           action_exception, 3040020, "Invalid fungible asset symbol" );
FC_DECLARE_DERIVED_EXCEPTION( fungible_supply_exception,           action_exception, 3040021, "Invalid fungible supply" );
FC_DECLARE_DERIVED_EXCEPTION( fungible_address_exception,          action_exception, 3040022, "Invalid address" );
FC_DECLARE_DERIVED_EXCEPTION( token_owner_exception,               action_exception, 3040023, "Token owner cannot be empty." );
FC_DECLARE_DERIVED_EXCEPTION( token_destoryed_exception,           action_exception, 3040024, "Token is destroyed." );
FC_DECLARE_DERIVED_EXCEPTION( math_overflow_exception,             action_exception, 3040025, "Operations resulted in overflow." );
FC_DECLARE_DERIVED_EXCEPTION( balance_exception,                   action_exception, 3040026, "Not enough balance left." );
FC_DECLARE_DERIVED_EXCEPTION( meta_involve_exception,              action_exception, 3040027, "Creator is not involved." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_status_exception,            action_exception, 3040028, "Suspend transaction is not in proper status." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_proposer_key_exception,      action_exception, 3040029, "Proposer needs to sign his key." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_duplicate_key_exception,     action_exception, 3040030, "Some keys are already signed this suspend transaction." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_expired_tx_exception,        action_exception, 3040031, "Suspend transaction is expired." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_not_required_keys_exception, action_exception, 3040032, "Provided keys are not required." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_executor_exception,          action_exception, 3040033, "Invalid executor." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_invalid_action_exception,    action_exception, 3040034, "Action is not valid for suspend transaction." );
FC_DECLARE_DERIVED_EXCEPTION( name_reserved_exception,             action_exception, 3040035, "Name is reserved." );
FC_DECLARE_DERIVED_EXCEPTION( evt_link_exception,                  action_exception, 3040036, "EVT-Link is not valid." );
FC_DECLARE_DERIVED_EXCEPTION( evt_link_no_key_exception,           action_exception, 3040037, "Specific segment key is not in this evt-link." );
FC_DECLARE_DERIVED_EXCEPTION( evt_link_version_exception,          action_exception, 3040038, "EVT-Link version is not valid.");
FC_DECLARE_DERIVED_EXCEPTION( evt_link_id_exception,               action_exception, 3040039, "EVT-Link id is not valid.");
FC_DECLARE_DERIVED_EXCEPTION( evt_link_dupe_exception,             action_exception, 3040040, "Duplicate EVT-Link.");
FC_DECLARE_DERIVED_EXCEPTION( evt_link_type_exception,             action_exception, 3040041, "Invalid EVT-Link type.");
FC_DECLARE_DERIVED_EXCEPTION( evt_link_expiration_exception,       action_exception, 3040042, "EVT-Link is expired.");
FC_DECLARE_DERIVED_EXCEPTION( evt_link_existed_exception,          action_exception, 3040043, "EVT-Link is not existed.");
FC_DECLARE_DERIVED_EXCEPTION( everipass_exception,                 action_exception, 3040044, "everiPass failed.");
FC_DECLARE_DERIVED_EXCEPTION( everipay_exception,                  action_exception, 3040045, "everiPay failed.");
FC_DECLARE_DERIVED_EXCEPTION( prodvote_key_exception,              action_exception, 3040046, "Unknown prodvote conf key.");
FC_DECLARE_DERIVED_EXCEPTION( prodvote_value_exception,            action_exception, 3040047, "Invalid prodvote conf value.");
FC_DECLARE_DERIVED_EXCEPTION( prodvote_producer_exception,         action_exception, 3040048, "Invalid producer.");

FC_DECLARE_DERIVED_EXCEPTION( name_type_exception,               chain_type_exception, 3120001, "Invalid name" );
FC_DECLARE_DERIVED_EXCEPTION( public_key_type_exception,         chain_type_exception, 3120002, "Invalid public key" );
FC_DECLARE_DERIVED_EXCEPTION( private_key_type_exception,        chain_type_exception, 3120003, "Invalid private key" );
FC_DECLARE_DERIVED_EXCEPTION( authority_type_exception,          chain_type_exception, 3120004, "Invalid authority" );
FC_DECLARE_DERIVED_EXCEPTION( action_type_exception,             chain_type_exception, 3120005, "Invalid action" );
FC_DECLARE_DERIVED_EXCEPTION( transaction_type_exception,        chain_type_exception, 3120006, "Invalid transaction" );
FC_DECLARE_DERIVED_EXCEPTION( abi_type_exception,                chain_type_exception, 3120007, "Invalid ABI" );
FC_DECLARE_DERIVED_EXCEPTION( abi_not_found_exception,           chain_type_exception, 3120008, "No ABI found" );
FC_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception, 3120009, "Invalid block ID" );
FC_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception, 3120010, "Invalid transaction ID" );
FC_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception, 3120011, "Invalid packed transaction" );
FC_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception, 3120012, "Invalid asset" );
FC_DECLARE_DERIVED_EXCEPTION( symbol_type_exception,             chain_type_exception, 3120013, "Invalid symbol" );
FC_DECLARE_DERIVED_EXCEPTION( permission_type_exception,         chain_type_exception, 3120014, "Invalid permission" );
FC_DECLARE_DERIVED_EXCEPTION( group_type_exception,              chain_type_exception, 3120015, "Invalid group" );
FC_DECLARE_DERIVED_EXCEPTION( authorizer_ref_type_exception,     chain_type_exception, 3120016, "Invalid authorizer ref" );
FC_DECLARE_DERIVED_EXCEPTION( address_type_exception,            chain_type_exception, 3120017, "Invalid address" );
FC_DECLARE_DERIVED_EXCEPTION( name128_type_exception,            chain_type_exception, 3120018, "Invalid name128" );
FC_DECLARE_DERIVED_EXCEPTION( chain_id_type_exception,           chain_type_exception, 3120019, "Invalid chain id" );

FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,   plugin_exception, 3130001, "Missing Chain API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,  plugin_exception, 3130002, "Missing Wallet API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,     plugin_exception, 3130003, "Missing Net API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_evt_api_plugin_exception,     plugin_exception, 3130004, "Missing EVT API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_history_api_plugin_exception, plugin_exception, 3130005, "Missing History API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( plugin_config_exception,              plugin_exception, 3130006, "Incorrect plugin configuration" );
FC_DECLARE_DERIVED_EXCEPTION( mongodb_plugin_not_enabled_exception, plugin_exception, 3130007, "Mongodb plugin is not enabled" );

FC_DECLARE_DERIVED_EXCEPTION( invalid_http_client_root_cert,    http_exception, 3180001, "invalid http client root certificate" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_http_response,            http_exception, 3180002, "invalid http response" );
FC_DECLARE_DERIVED_EXCEPTION( resolved_to_multiple_ports,       http_exception, 3180003, "service resolved to multiple ports" );
FC_DECLARE_DERIVED_EXCEPTION( fail_to_resolve_host,             http_exception, 3180004, "fail to resolve host" );
FC_DECLARE_DERIVED_EXCEPTION( http_request_fail,                http_exception, 3180005, "http request fail" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_http_request,             http_exception, 3180006, "invalid http request" );
FC_DECLARE_DERIVED_EXCEPTION( exceed_deferred_request,          http_exception, 3180007, "exceed max http deferred request" );
FC_DECLARE_DERIVED_EXCEPTION( alloc_deferred_fail,              http_exception, 3180008, "Failed to alloc deferred id" );

FC_DECLARE_DERIVED_EXCEPTION( wallet_exist_exception,            wallet_exception, 3140001, "Wallet already exists" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_nonexistent_exception,      wallet_exception, 3140002, "Nonexistent wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_locked_exception,           wallet_exception, 3140003, "Locked wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_missing_pub_key_exception,  wallet_exception, 3140004, "Missing public key" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_invalid_password_exception, wallet_exception, 3140005, "Invalid wallet password" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_not_available_exception,    wallet_exception, 3140006, "No available wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_unlocked_exception,         wallet_exception, 3140007, "Already unlocked" );
FC_DECLARE_DERIVED_EXCEPTION( key_exist_exception,               wallet_exception, 3140008, "Key already exists" );
FC_DECLARE_DERIVED_EXCEPTION( key_nonexistent_exception,         wallet_exception, 3140009, "Nonexistent key" );
FC_DECLARE_DERIVED_EXCEPTION( unsupported_key_type_exception,    wallet_exception, 3140010, "Unsupported key type" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_lock_timeout_exception,    wallet_exception, 3140011, "Wallet lock timeout is invalid" );
FC_DECLARE_DERIVED_EXCEPTION( secure_enclave_exception,          wallet_exception, 3140012, "Secure Enclave Exception" );

FC_DECLARE_DERIVED_EXCEPTION( tokendb_domain_existed,            tokendb_exception, 3150001, "Domain already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_group_existed,             tokendb_exception, 3150002, "Permission Group already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_token_existed,             tokendb_exception, 3150003, "Token already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_account_existed,           tokendb_exception, 3150004, "Account already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_suspend_existed,           tokendb_exception, 3150005, "Delay already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_domain_not_found,          tokendb_exception, 3150006, "Not found specific domain" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_group_not_found,           tokendb_exception, 3150007, "Not found specific group" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_token_not_found,           tokendb_exception, 3150008, "Not found specific token" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_suspend_not_found,         tokendb_exception, 3150009, "Not found specific suspend" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_fungible_not_found,        tokendb_exception, 3150010, "Not found specific fungible" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_address_not_found,         tokendb_exception, 3150011, "Not found specific address" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_asset_not_found,           tokendb_exception, 3150012, "Not found specific asset in address" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_rocksdb_exception,         tokendb_exception, 3150013, "Rocksdb internal error occurred." );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_no_savepoint,              tokendb_exception, 3150014, "No savepoints anymore" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_seq_not_valid,             tokendb_exception, 3150015, "Seq for checkpoint is not valid." );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_db_action_exception,       tokendb_exception, 3150016, "Unknown db action type." );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_dirty_flag_exception,      tokendb_exception, 3150017, "Checkspoints log file is in dirty." );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_squash_exception,          tokendb_exception, 3150018, "Cannot perform squash operation now" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_prodvote_not_found,        tokendb_exception, 3150019, "Not found specific producer vote" );

FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           misc_exception, 3100002, "unknown block" );
FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     misc_exception, 3100003, "unknown transaction" );
FC_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,     misc_exception, 3100004, "Corrupted reversible block database was fixed." );
FC_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,   misc_exception, 3100005, "extracted genesis state from blocks.log" );
FC_DECLARE_DERIVED_EXCEPTION( unsupported_feature,               misc_exception, 3100006, "Feature is currently unsupported" );
FC_DECLARE_DERIVED_EXCEPTION( node_management_success,           misc_exception, 3100007, "Node management operation successfully executed" );

FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                 authorization_exception, 3090001, "Duplicate signature is included." );
FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                authorization_exception, 3090002, "Irrelevant signature is included." );
FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,        authorization_exception, 3090003, "Provided keys do not satisfy declared authorizations." );

}} // evt::chain
