/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/jmzk_contract_abi.hpp>
#include <jmzk/chain/contracts/types.hpp>
#include <jmzk/chain/contracts/abi_types.hpp>
#include <jmzk/chain/version.hpp>

namespace jmzk { namespace chain { namespace contracts {

/**
 * Hisotry
 * 4.1.1: Update memo field in everipass v2 and everipay v2 to be optional
 */

static auto jmzk_abi_version       = 4;
static auto jmzk_abi_minor_version = 1;
static auto jmzk_abi_patch_version = 1;

version
jmzk_contract_abi_version() {
    return version(jmzk_abi_version, jmzk_abi_minor_version, jmzk_abi_patch_version);
}

abi_def
jmzk_contract_abi() {
    auto jmzk_abi = abi_def();

    jmzk_abi.types.push_back(type_def{"address_list", "address[]"});
    jmzk_abi.types.push_back(type_def{"user_id", "public_key"});
    jmzk_abi.types.push_back(type_def{"user_list", "public_key[]"});
    jmzk_abi.types.push_back(type_def{"group_key", "public_key"});
    jmzk_abi.types.push_back(type_def{"weight_type", "uint16"});
    jmzk_abi.types.push_back(type_def{"fields", "field_def[]"});
    jmzk_abi.types.push_back(type_def{"type_name", "string"});
    jmzk_abi.types.push_back(type_def{"field_name", "string"});
    jmzk_abi.types.push_back(type_def{"permission_name", "name"});
    jmzk_abi.types.push_back(type_def{"action_name", "name"});
    jmzk_abi.types.push_back(type_def{"domain_name", "name128"});
    jmzk_abi.types.push_back(type_def{"domain_key", "name128"});
    jmzk_abi.types.push_back(type_def{"group_name", "name128"});
    jmzk_abi.types.push_back(type_def{"token_name", "name128"});
    jmzk_abi.types.push_back(type_def{"account_name", "name128"});
    jmzk_abi.types.push_back(type_def{"proposal_name", "name128"});
    jmzk_abi.types.push_back(type_def{"fungible_name", "name128"});
    jmzk_abi.types.push_back(type_def{"symbol_name", "name128"});
    jmzk_abi.types.push_back(type_def{"symbol_id_type", "uint32"});
    jmzk_abi.types.push_back(type_def{"balance_type", "asset"});
    jmzk_abi.types.push_back(type_def{"group_def", "group"});
    jmzk_abi.types.push_back(type_def{"meta_key", "name128"});
    jmzk_abi.types.push_back(type_def{"meta_value", "string"});
    jmzk_abi.types.push_back(type_def{"suspend_status", "uint8"});
    jmzk_abi.types.push_back(type_def{"conf_key", "name128"});

    // enums def
    jmzk_abi.enums.emplace_back( enum_def {
        "passive_method_type", "uint8", {
            "within_amount",
            "outside_amount"
        }
    });

    jmzk_abi.enums.emplace_back( enum_def {
        "stake_type", "uint64", {
            "active",
            "fixed"
        }
    });

    jmzk_abi.enums.emplace_back( enum_def {
        "unstake_op", "uint64", {
            "propose",
            "cancel",
            "settle"
        }
    });

    // structures def
    jmzk_abi.structs.emplace_back( struct_def {
        "void", "", {}
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "key_weight", "", {
            {"key", "public_key"},
            {"weight", "weight_type"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "authorizer_weight", "", {
            {"ref", "authorizer_ref"},
            {"weight", "weight_type"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "permission_def", "", {
            {"name", "permission_name"},
            {"threshold", "uint32"},
            {"authorizers", "authorizer_weight[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "locknft_def", "", {
            {"domain", "domain_name"},
            {"names", "token_name[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "lockft_def", "", {
            {"from", "address"},
            {"amount", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "lock_condkeys", "", {
            {"threshold", "uint16"},
            {"cond_keys", "public_key[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "dist_stack_receiver", "", {
            {"threshold", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "dist_fixed_rule", "", {
            {"receiver", "dist_receiver"},
            {"amount", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "dist_percent_rule", "", {
            {"receiver", "dist_receiver"},
            {"percent", "percent"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "dist_percent_rule_v2", "", {
            {"receiver", "dist_receiver"},
            {"percent", "percent_slim"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "passive_method", "", {
            {"action", "name"},
            {"method", "passive_method_type"}
        }
    });

    // variants def
    jmzk_abi.variants.emplace_back( variant_def {
        "lock_asset", {
            {"tokens", "locknft_def"},
            {"fungible", "lockft_def"}
        }
    });

    jmzk_abi.variants.emplace_back( variant_def {
        "lock_condition", {
            {"cond_keys", "lock_condkeys"}
        }
    });

    jmzk_abi.variants.emplace_back( variant_def {
        "lock_aprvdata", {
            {"cond_key", "void"}
        }
    });

    jmzk_abi.variants.emplace_back( variant_def {
        "dist_receiver", {
            {"address", "address"},
            {"ftholders", "dist_stack_receiver"}
        }
    });

    jmzk_abi.variants.emplace_back( variant_def {
        "dist_rule", {
            {"fixed", "dist_fixed_rule"},
            {"percent", "dist_percent_rule"},
            {"remaining_percent", "dist_percent_rule"}
        }
    });

    jmzk_abi.variants.emplace_back( variant_def {
        "dist_rule_v2", {
            {"fixed", "dist_fixed_rule"},
            {"percent", "dist_percent_rule_v2"},
            {"remaining_percent", "dist_percent_rule_v2"}
        }
    });

    // actions def
    jmzk_abi.structs.emplace_back( struct_def {
        "newdomain", "", {
            {"name", "domain_name"},
            {"creator", "user_id"},
            {"issue", "permission_def"},
            {"transfer", "permission_def"},
            {"manage", "permission_def"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "issuetoken", "", {
            {"domain", "domain_name"},
            {"names", "token_name[]"},
            {"owner", "address_list"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "transfer", "", {
            {"domain", "domain_name"},
            {"name", "token_name"},
            {"to", "address_list"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "destroytoken", "", {
            {"domain", "domain_name"},
            {"name", "token_name"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newgroup", "", {
            {"name", "group_name"},
            {"group", "group_def"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updategroup", "", {
            {"name", "group_name"},
            {"group", "group_def"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updatedomain", "", {
            {"name", "domain_name"},
            {"issue", "permission_def?"},
            {"transfer", "permission_def?"},
            {"manage", "permission_def?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newfungible", "", {
            {"name", "fungible_name"},
            {"sym_name", "symbol_name"},
            {"sym", "symbol"},
            {"creator", "user_id"},
            {"issue", "permission_def"},
            {"manage", "permission_def"},
            {"total_supply", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newfungible_v2", "", {
            {"name", "fungible_name"},
            {"sym_name", "symbol_name"},
            {"sym", "symbol"},
            {"creator", "user_id"},
            {"issue", "permission_def"},
            {"transfer", "permission_def"},
            {"manage", "permission_def"},
            {"total_supply", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updfungible", "", {
            {"sym_id", "symbol_id_type"},
            {"issue", "permission_def?"},
            {"manage", "permission_def?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updfungible_v2", "", {
            {"sym_id", "symbol_id_type"},
            {"issue", "permission_def?"},
            {"transfer", "permission_def?"},
            {"manage", "permission_def?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "issuefungible", "", {
            {"address", "address"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "transferft", "", {
            {"from", "address"},
            {"to", "address"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "recycleft", "", {
            {"address", "address"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "destroyft", "", {
            {"address", "address"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "jmzk2pjmzk", "", {
            {"from", "address"},
            {"to", "address"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "addmeta", "", {
            {"key", "meta_key"},
            {"value", "meta_value"},
            {"creator", "authorizer_ref"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newsuspend", "", {
            {"name", "proposal_name"},
            {"proposer", "user_id"},
            {"trx", "transaction"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "cancelsuspend", "", {
            {"name", "proposal_name"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "aprvsuspend", "", {
            {"name", "proposal_name"},
            {"signatures", "signature[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "execsuspend", "", {
           {"name", "proposal_name"},
           {"executor", "user_id"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "paycharge", "", {
           {"payer", "address"},
           {"charge", "uint32"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "paybonus", "", {
           {"payer", "address"},
           {"amount", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "everipass", "", {
           {"link", "jmzk_link"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "everipass_v2", "", {
           {"link", "jmzk_link"},
           {"memo", "string?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "everipay", "", {
           {"link", "jmzk_link"},
           {"payee", "address"},
           {"number", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "everipay_v2", "", {
           {"link", "jmzk_link"},
           {"payee", "address"},
           {"number", "asset"},
           {"memo", "string?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "prodvote", "", {
           {"producer", "account_name"},
           {"key", "conf_key"},
           {"value", "int64"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "producer_key", "", {
           {"producer_name", "account_name"},
           {"block_signing_key", "public_key"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updsched", "", {
           {"producers", "producer_key[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newlock", "", {
            {"name", "proposal_name"},
            {"proposer", "user_id"},
            {"unlock_time", "time_point_sec"},
            {"deadline","time_point_sec"},
            {"assets","lock_asset[]"},
            {"condition", "lock_condition"},
            {"succeed", "address[]"},
            {"failed", "address[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "aprvlock", "", {
            {"name", "proposal_name"},
            {"approver", "user_id"},
            {"data", "lock_aprvdata"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "tryunlock", "", {
           {"name", "proposal_name"},
           {"executor", "user_id"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "setpsvbonus", "", {
           {"sym", "symbol"},
           {"rate", "percent"},
           {"base_charge", "asset"},
           {"charge_threshold", "asset?"},
           {"minimum_charge", "asset?"},
           {"dist_threshold", "asset"},
           {"rules", "dist_rule[]"},
           {"methods", "passive_method[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "setpsvbonus_v2", "", {
           {"sym_id", "symbol_id_type"},
           {"rate", "percent_slim"},
           {"base_charge", "asset"},
           {"charge_threshold", "asset?"},
           {"minimum_charge", "asset?"},
           {"dist_threshold", "asset"},
           {"rules", "dist_rule_v2[]"},
           {"methods", "passive_method[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "distpsvbonus", "", {
           {"sym_id", "symbol_id_type"},
           {"deadline", "time_point"},
           {"final_receiver", "address?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newstakepool", "", {
           {"sym_id", "symbol_id_type"},
           {"purchase_threshold", "asset"},
           {"demand_r", "int32"},
           {"demand_t", "int32"},
           {"demand_q", "int32"},
           {"demand_w", "int32"},
           {"fixed_r", "int32"},
           {"fixed_t", "int32"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "updstakepool", "", {
           {"sym_id", "symbol_id_type"},
           {"purchase_threshold", "asset?"},
           {"demand_r", "int32?"},
           {"demand_t", "int32?"},
           {"demand_q", "int32?"},
           {"demand_w", "int32?"},
           {"fixed_r", "int32?"},
           {"fixed_t", "int32?"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "newvalidator", "", {
           {"name", "account_name"},
           {"creator", "user_id"},
           {"signer", "public_key"},
           {"withdraw", "permission_def"},
           {"manage", "permission_def"},
           {"commission", "percent_slim"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "staketkns", "", {
           {"staker", "user_id"},
           {"validator", "account_name"},
           {"amount", "asset"},
           {"type", "stake_type"},
           {"fixed_days", "int32"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "unstaketkns", "", {
           {"staker", "user_id"},
           {"validator", "account_name"},
           {"units", "int64"},
           {"sym_id", "symbol_id_type"},
           {"op", "unstake_op"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "toactivetkns", "", {
           {"staker", "user_id"},
           {"validator", "account_name"},
           {"sym_id", "symbol_id_type"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "valiwithdraw", "", {
           {"name", "account_name"},
           {"addr", "address"},
           {"amount", "asset"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "recvstkbonus", "", {
           {"validator", "account_name"},
           {"sym_id", "symbol_id_type"}
        }
    });

    // abi_def fields
    jmzk_abi.structs.emplace_back( struct_def {
        "field_def", "", {
            {"name", "field_name"},
            {"type", "type_name"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "struct_def", "", {
            {"name", "type_name"},
            {"base", "type_name"},
            {"fields", "fields"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "type_def", "", {
            {"new_type_name", "type_name"},
            {"type", "type_name"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "action_def", "", {
           {"name", "action_name"},
           {"type", "type_name"}
        }
    });

    // blocks & transactions def
    jmzk_abi.structs.emplace_back( struct_def {
        "action", "", {
            {"name", "action_name"},
            {"domain", "domain_name"},
            {"key", "domain_key"},
            {"data", "bytes"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "transaction_header", "", {
            {"expiration", "time_point_sec"},
            {"ref_block_num", "uint16"},
            {"ref_block_prefix", "uint32"},
            {"max_charge", "uint32"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "transaction", "transaction_header", {
            {"actions", "action[]"},
            {"payer", "address"},
            {"transaction_extensions", "extensions"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "signed_transaction", "transaction", {
            {"signatures", "signature[]"}
        }
    });

    jmzk_abi.structs.emplace_back( struct_def {
        "block_header", "", {
            {"previous", "checksum256"},
            {"timestamp", "uint32"},
            {"transaction_mroot", "checksum256"},
            {"action_mroot", "checksum256"},
            {"block_mroot", "checksum256"},
            {"producer", "account_name"},
            {"schedule_version", "uint32"},
            {"new_producers", "producer_schedule?"}
        }
    });

    return jmzk_abi;
}

}}}  // namespace jmzk::chain::contracts