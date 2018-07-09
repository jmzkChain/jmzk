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
FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          chain_exception, 3010000, "Database query exception" );
FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          chain_exception, 3020000, "block validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception, 3030000, "transaction validation exception" );
FC_DECLARE_DERIVED_EXCEPTION( action_exception,                  chain_exception, 3040000, "action validation exception" );
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

FC_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,         block_validate_exception, 3020001, "Transaction outputs in block do not match transaction outputs from applying block." );
FC_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception,       block_validate_exception, 3020002, "Block does not guarantee concurrent executions without conflicts." );
FC_DECLARE_DERIVED_EXCEPTION( block_lock_exception,              block_validate_exception, 3020003, "Shard locks in block are incorrect or mal-formed." );
FC_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,          block_validate_exception, 3020004, "Block exhausted allowed resources." );
FC_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,           block_validate_exception, 3020005, "Block is too old to push." );

FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                      transaction_exception, 3030001, "duplicate transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,            transaction_exception, 3030002, "Error decompressing transaction" );
FC_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,              transaction_exception, 3030003, "Expired Transaction" );
FC_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,          transaction_exception, 3030004, "Transaction Expiration Too Far" );
FC_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception,       transaction_exception, 3030005, "Invalid Reference Block" );
FC_DECLARE_DERIVED_EXCEPTION( tx_apply_exception,                transaction_exception, 3030006, "Transaction Apply Exception" );
FC_DECLARE_DERIVED_EXCEPTION( tx_receipt_inconsistent_status,    transaction_exception, 3030007, "Transaction receipt applied status does not match received status." );
FC_DECLARE_DERIVED_EXCEPTION( tx_no_action,                      transaction_exception, 3030008, "transaction should have at least one normal action." );
FC_DECLARE_DERIVED_EXCEPTION( deadline_exception,                transaction_exception, 3030009, "transaction is timeout.");

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
FC_DECLARE_DERIVED_EXCEPTION( token_owner_exception,               action_exception, 3040022, "Token owner cannot be empty." );
FC_DECLARE_DERIVED_EXCEPTION( token_destoryed_exception,           action_exception, 3040023, "Token is destroyed." );
FC_DECLARE_DERIVED_EXCEPTION( math_overflow_exception,             action_exception, 3040024, "Operations resulted in overflow." );
FC_DECLARE_DERIVED_EXCEPTION( balance_exception,                   action_exception, 3040025, "Not enough balance left." );
FC_DECLARE_DERIVED_EXCEPTION( meta_involve_exception,              action_exception, 3040026, "Creator is not involved." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_status_exception,            action_exception, 3040027, "Suspend transaction is not in proper status." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_proposer_key_exception,      action_exception, 3040028, "Proposer needs to sign his key." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_duplicate_key_exception,     action_exception, 3040029, "Some keys are already signed this suspend transaction." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_expired_tx_exception,        action_exception, 3040030, "Suspend transaction is expired." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_not_required_keys_exception, action_exception, 3040031, "Provided keys are not required." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_executor_exception,          action_exception, 3040032, "Invalid executor." );
FC_DECLARE_DERIVED_EXCEPTION( suspend_invalid_action_exception,    action_exception, 3040033, "Action is not valid for suspend transaction." );
FC_DECLARE_DERIVED_EXCEPTION( name_reserved_exception,             action_exception, 3040034, "Name is reserved." );

FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   undo_database_exception, 3070001, "There are no blocks to be popped." );

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
FC_DECLARE_DERIVED_EXCEPTION( authorizer_ref_type_exception,     chain_type_exception, 3120015, "Invalid authorizer ref" );

FC_DECLARE_DERIVED_EXCEPTION( missing_chain_api_plugin_exception,   missing_plugin_exception, 3130001, "Missing Chain API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_wallet_api_plugin_exception,  missing_plugin_exception, 3130002, "Missing Wallet API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_net_api_plugin_exception,     missing_plugin_exception, 3130003, "Missing Net API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_evt_api_plugin_exception,     missing_plugin_exception, 3130004, "Missing EVT API Plugin" );
FC_DECLARE_DERIVED_EXCEPTION( missing_history_api_plugin_exception, missing_plugin_exception, 3130005, "Missing History API Plugin" );

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
FC_DECLARE_DERIVED_EXCEPTION( tokendb_suspend_existed,             tokendb_exception, 3150005, "Delay already exists" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_domain_not_found,          tokendb_exception, 3150006, "Not found specific domain" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_group_not_found,           tokendb_exception, 3150007, "Not found specific group" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_token_not_found,           tokendb_exception, 3150008, "Not found specific token" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_suspend_not_found,           tokendb_exception, 3150009, "Not found specific suspend" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_fungible_not_found,        tokendb_exception, 3150010, "Not found specific fungible" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_address_not_found,         tokendb_exception, 3150011, "Not found specific address" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_asset_not_found,           tokendb_exception, 3150012, "Not found specific asset in address" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_rocksdb_fail,              tokendb_exception, 3150013, "Rocksdb internal error occurred." );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_no_savepoint,              tokendb_exception, 3150014, "No savepoints anymore" );
FC_DECLARE_DERIVED_EXCEPTION( tokendb_seq_not_valid,             tokendb_exception, 3150015, "Seq for checkpoint is not valid." );

FC_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,           misc_exception, 3100002, "unknown block" );
FC_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,     misc_exception, 3100003, "unknown transaction" );
FC_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,     misc_exception, 3100004, "Corrupted reversible block database was fixed." );
FC_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,   misc_exception, 3100005, "extracted genesis state from blocks.log" );

FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                 authorization_exception, 3090001, "Duplicate signature is included." );
FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                authorization_exception, 3090002, "Irrelevant signature is included." );
FC_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,        authorization_exception, 3090003, "Provided keys do not satisfy declared authorizations." );

}} // evt::chain
