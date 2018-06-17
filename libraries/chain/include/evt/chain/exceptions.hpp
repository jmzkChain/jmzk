/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <boost/core/typeinfo.hpp>
#include <evt/chain/protocol.hpp>
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
    catch(chain_exception & e) {                                                                \
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
    catch(chain_exception & e) {                                                                       \
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
FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          chain_exception, 3010000, "database query exception" );
FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          chain_exception, 3020000, "block validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception, 3030000, "transaction validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception,         chain_exception, 3040000, "message validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 chain_exception, 3070000, "utility method exception" );
FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           chain_exception, 3080000, "undo database exception" );
FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        chain_exception, 3090000, "unlinkable block" );
FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              chain_exception, 3100000, "black swan" );
FC_DECLARE_DERIVED_EXCEPTION( chain_type_exception,              chain_exception, 3110000, "chain type exception" );
FC_DECLARE_DERIVED_EXCEPTION( missing_plugin_exception,          chain_exception, 3120000, "missing plugin exception" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_exception,                  chain_exception, 3130000, "wallet exception" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_exception,                 chain_exception, 3140000, "tokendb exception" );
FC_DECLARE_DERIVED_EXCEPTION( misc_exception,                    chain_exception, 3150000, "Miscellaneous exception");
FC_DECLARE_DERIVED_EXCEPTION( authorization_exception,           chain_exception, 3160000, "Authorization exception");

FC_DECLARE_DERIVED_EXCEPTION( permission_query_exception,        database_query_exception, 3010001, "Permission Query Exception" );
FC_DECLARE_DERIVED_EXCEPTION( account_query_exception,           database_query_exception, 3010002, "Account Query Exception" );
FC_DECLARE_DERIVED_EXCEPTION( contract_table_query_exception,    database_query_exception, 3010003, "Contract Table Query Exception" );
FC_DECLARE_DERIVED_EXCEPTION( contract_query_exception,          database_query_exception, 3010004, "Contract Query Exception" );

FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         block_validate_exception, 3020001, "transaction outputs in block do not match transaction outputs from applying block" );
FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception,       block_validate_exception, 3020002, "block does not guarantee concurrent exection without conflicts" );
FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,              block_validate_exception, 3020003, "shard locks in block are incorrect or mal-formed" );
FC_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,          block_validate_exception, 3020004, "block exhausted allowed resources" );
FC_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,           block_validate_exception, 3020005, "block is too old to push" );

FC_DECLARE_DERIVED_EXCEPTION( tx_missing_auth,                   transaction_exception, 3030001, "missing required authority" );
FC_DECLARE_DERIVED_EXCEPTION( tx_missing_sigs,                   transaction_exception, 3030002, "signatures do not satisfy declared authorizations" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        transaction_exception, 3030006, "committee account cannot directly approve transaction" );
FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  transaction_exception, 3030007, "insufficient fee" );
FC_DECLARE_DERIVED_EXCEPTION( tx_missing_recipient,              transaction_exception, 3030009, "missing required recipient" );
FC_DECLARE_DERIVED_EXCEPTION( checktime_exceeded,                transaction_exception, 3030010, "allotted processing time was exceeded" );
FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      transaction_exception, 3030011, "duplicate transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_scheduling_exception,           transaction_exception, 3030013, "transaction failed during sheduling" );
FC_DECLARE_DERIVED_EXCEPTION( tx_unknown_argument,               transaction_exception, 3030014, "transaction provided an unknown value to a system call" );
FC_DECLARE_DERIVED_EXCEPTION( tx_resource_exhausted,             transaction_exception, 3030015, "transaction exhausted allowed resources" );
FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,                 transaction_exception, 3030016, "error in WASM page memory" );
FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_permission,            transaction_exception, 3030017, "Unsatisfied permission" );
FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_auth_exceeded,             transaction_exception, 3030018, "Number of transaction messages per authorized account has been exceeded" );
FC_DECLARE_DERIVED_EXCEPTION( tx_msgs_code_exceeded,             transaction_exception, 3030019, "Number of transaction messages per code account has been exceeded" );
FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,              transaction_exception, 3030020, "Runtime Error Processing WASM" );
FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,            transaction_exception, 3030021, "Error decompressing transaction" );
FC_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,              transaction_exception, 3030022, "Expired Transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,          transaction_exception, 3030023, "Transaction Expiration Too Far" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception,       transaction_exception, 3030024, "Invalid Reference Block" );
FC_DECLARE_DERIVED_EXCEPTION( tx_apply_exception,                transaction_exception, 3030025, "Transaction Apply Exception" );
FC_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error,          transaction_exception, 3030026, "Serialization Error Processing WASM" );
FC_DECLARE_DERIVED_EXCEPTION( tx_empty_region,                   transaction_exception, 3030027, "Transaction contains an empty region" );
FC_DECLARE_DERIVED_EXCEPTION( tx_empty_cycle,                    transaction_exception, 3030028, "Transaction contains an empty cycle" );
FC_DECLARE_DERIVED_EXCEPTION( tx_empty_shard,                    transaction_exception, 3030029, "Transaction contains an empty shard" );
FC_DECLARE_DERIVED_EXCEPTION( tx_receipt_inconsistent_status,    transaction_exception, 3030030, "Transaction receipt applied status does not match received status" );
FC_DECLARE_DERIVED_EXCEPTION( cfa_irrelevant_auth,               transaction_exception, 3030031, "context-free action should have no required authority" );
FC_DECLARE_DERIVED_EXCEPTION( tx_no_action,                      transaction_exception, 3030032, "transaction should have at least one normal action" );
FC_DECLARE_DERIVED_EXCEPTION( tx_no_auths,                       transaction_exception, 3030033, "transaction should have at least one required authority" );
FC_DECLARE_DERIVED_EXCEPTION( tx_receipt_inconsistent_cpu,       transaction_exception, 3030034, "Transaction receipt applied kcpu_usage does not match received kcpu_usage" );
FC_DECLARE_DERIVED_EXCEPTION( tx_receipt_inconsistent_net,       transaction_exception, 3030035, "Transaction receipt applied net_usage_words does not match received net_usage_words" );
FC_DECLARE_DERIVED_EXCEPTION( serialization_exception,           transaction_exception, 3030036, "serialization exception");
FC_DECLARE_DERIVED_EXCEPTION( deserialization_exception,         transaction_exception, 3030037, "deserialization exception");
FC_DECLARE_DERIVED_EXCEPTION( deadline_exception,                transaction_exception, 3030038, "transaction took too long");

FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception,     action_validate_exception, 3040001, "account name already exists" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception,     action_validate_exception, 3040002, "Invalid Action Arguments" );
FC_DECLARE_DERIVED_EXCEPTION( evt_assert_message_exception,      action_validate_exception, 3040003, "evt_assert_message assertion failure" );
FC_DECLARE_DERIVED_EXCEPTION( evt_assert_code_exception,         action_validate_exception, 3040004, "evt_assert_code assertion failure" );

FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   undo_database_exception, 3070001, "there are no blocks to pop" );

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
FC_DECLARE_DERIVED_EXCEPTION( permission_type_exception,         chain_type_exception, 3120013, "Invalid permission" );
FC_DECLARE_DERIVED_EXCEPTION( group_type_exception,              chain_type_exception, 3120014, "Invalid group" );

FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,  missing_plugin_exception, 3130001, "Missing Chain API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception, missing_plugin_exception, 3130002, "Missing Wallet API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,    missing_plugin_exception, 3130003, "Missing Net API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_evt_api_plugin_exception,    missing_plugin_exception, 3130004, "Missing EVT API Plugin" );

FC_DECLARE_DERIVED_EXCEPTION( wallet_exist_exception,            wallet_exception, 3140001, "Wallet already exists" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_nonexistent_exception,      wallet_exception, 3140002, "Nonexistent wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_locked_exception,           wallet_exception, 3140003, "Locked wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_missing_pub_key_exception,  wallet_exception, 3140004, "Missing public key" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_invalid_password_exception, wallet_exception, 3140005, "Invalid wallet password" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_not_available_exception,    wallet_exception, 3140006, "No available wallet" );
FC_DECLARE_DERIVED_EXCEPTION( wallet_unlocked_exception,         wallet_exception, 3140007, "Already unlocked" );

FC_DECLARE_DERIVED_EXCEPTION( tokendb_domain_existed,            tokendb_exception, 3150001, "Domain already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_group_existed,             tokendb_exception, 3150002, "Permission Group already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_token_existed,             tokendb_exception, 3150003, "Token already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_account_existed,           tokendb_exception, 3150004, "Account already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_delay_existed,             tokendb_exception, 3150005, "Delay already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_domain_not_found,          tokendb_exception, 3150006, "Not found specifc domain" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_group_not_found,           tokendb_exception, 3150007, "Not found specific permission group" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_token_not_found,           tokendb_exception, 3150008, "Not found specific token" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_account_not_found,         tokendb_exception, 3150009, "Not found specific account" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_delay_not_found,           tokendb_exception, 3150010, "Not found specific delay" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_rocksdb_fail,              tokendb_exception, 3150011, "Rocksdb internal error occurred" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_no_savepoint,              tokendb_exception, 3150012, "No savepoints anymore" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_seq_not_valid,             tokendb_exception, 3150013, "Seq for checkpoint is not valid" );

FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           misc_exception, 3100002, "unknown block" );
FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     misc_exception, 3100003, "unknown transaction" );
FC_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,     misc_exception, 3100004, "corrupted reversible block database was fixed" );
FC_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,   misc_exception, 3100005, "extracted genesis state from blocks.log" );

FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                 authorization_exception, 3090001, "duplicate signature included" );
FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                authorization_exception, 3090002, "irrelevant signature included" );
FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,        authorization_exception, 3090003, "provided keys, permissions, and delays do not satisfy declared authorizations" );
FC_DECLARE_DERIVED_EXCEPTION( missing_auth_exception,           authorization_exception, 3090004, "missing required authority" );
FC_DECLARE_DERIVED_EXCEPTION( irrelevant_auth_exception,        authorization_exception, 3090005, "irrelevant authority included" );
FC_DECLARE_DERIVED_EXCEPTION( insufficient_delay_exception,     authorization_exception, 3090006, "insufficient delay" );

}} // evt::chain
