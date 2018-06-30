/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/types.hpp>

namespace evt { namespace chain { namespace contracts {

static auto evt_abi_version       = 2;
static auto evt_abi_minor_version = 0;
static auto evt_abi_patch_version = 0;

version
evt_contract_abi_version() {
    return version(evt_abi_version, evt_abi_minor_version, evt_abi_patch_version);
}

abi_def
evt_contract_abi() {
    abi_def evt_abi;
    evt_abi.types.push_back( type_def{"user_id","public_key"} );
    evt_abi.types.push_back( type_def{"user_list","public_key[]"} );
    evt_abi.types.push_back( type_def{"group_key","public_key"} );
    evt_abi.types.push_back( type_def{"weight_type","uint16"} );
    evt_abi.types.push_back( type_def{"fields","field_def[]"} );
    evt_abi.types.push_back( type_def{"type_name","string"} );
    evt_abi.types.push_back( type_def{"field_name","string"} );
    evt_abi.types.push_back( type_def{"permission_name","name"} );
    evt_abi.types.push_back( type_def{"action_name","name"} );
    evt_abi.types.push_back( type_def{"domain_name","name128"} );
    evt_abi.types.push_back( type_def{"domain_key","name128"} );
    evt_abi.types.push_back( type_def{"group_name","name128"} );
    evt_abi.types.push_back( type_def{"token_name","name128"} );
    evt_abi.types.push_back( type_def{"account_name","name128"} );
    evt_abi.types.push_back( type_def{"proposal_name","name128"} );
    evt_abi.types.push_back( type_def{"fungible_name","name128"} );
    evt_abi.types.push_back( type_def{"balance_type","asset"} );
    evt_abi.types.push_back( type_def{"group_def","group"} );
    evt_abi.types.push_back( type_def{"meta_key","name128"} );
    evt_abi.types.push_back( type_def{"meta_value","string"} );
    evt_abi.types.push_back( type_def{"meta_list","meta[]"} );
    evt_abi.types.push_back( type_def{"delay_status","uint8"} );

    evt_abi.actions.push_back( action_def{name("newdomain"), "newdomain"} );
    evt_abi.actions.push_back( action_def{name("issuetoken"), "issuetoken"} );
    evt_abi.actions.push_back( action_def{name("transfer"), "transfer"} );
    evt_abi.actions.push_back( action_def{name("destroytoken"), "destroytoken"} );
    evt_abi.actions.push_back( action_def{name("newgroup"), "newgroup"} );
    evt_abi.actions.push_back( action_def{name("updategroup"), "updategroup"} );
    evt_abi.actions.push_back( action_def{name("updatedomain"), "updatedomain"} );
    evt_abi.actions.push_back( action_def{name("newfungible"), "newfungible"} );
    evt_abi.actions.push_back( action_def{name("updfungible"), "updfungible"} );
    evt_abi.actions.push_back( action_def{name("issuefungible"), "issuefungible"} );
    evt_abi.actions.push_back( action_def{name("transferft"), "transferft"} );
    evt_abi.actions.push_back( action_def{name("addmeta"), "addmeta"} );
    evt_abi.actions.push_back( action_def{name("newdelay"), "newdelay"} );
    evt_abi.actions.push_back( action_def{name("canceldelay"), "canceldelay"} );
    evt_abi.actions.push_back( action_def{name("approvedelay"), "approvedelay"} );
    evt_abi.actions.push_back( action_def{name("executedelay"), "executedelay"} );

    // structures def
    evt_abi.structs.emplace_back( struct_def {
        "meta", "", {
            {"key", "meta_key"},
            {"value", "meta_value"},
            {"creator", "user_id"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "token_def", "", {
            {"domain", "domain_name"},
            {"name", "token_name"},
            {"owner", "user_list"},
            {"metas", "meta_list"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "key_weight", "", {
            {"key", "public_key"},
            {"weight", "weight_type"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "authorizer_weight", "", {
            {"ref", "authorizer_ref"},
            {"weight", "weight_type"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "permission_def", "", {
            {"name", "permission_name"},
            {"threshold", "uint32"},
            {"authorizers", "authorizer_weight[]"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "domain_def", "", {
            {"name", "domain_name"},
            {"creator", "user_id"},
            {"create_time", "time_point_sec"},
            {"issue", "permission_def"},
            {"transfer", "permission_def"},
            {"manage", "permission_def"},
            {"metas", "meta_list"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "fungibal_def", "", {
            {"sym", "symbol"},
            {"creator", "user_id"},
            {"create_time", "time_point_sec"},
            {"issue", "permission_def"},
            {"manage", "permission_def"},
            {"total_supply", "asset"},
            {"current_supply", "asset"},
            {"metas", "meta_list"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "delay_def", "", {
            {"name", "proposal_name"},
            {"proposer", "user_id"},
            {"status", "delay_status"},
            {"trx", "transaction"},
            {"signed_keys","public_key[]"},
            {"signatures","signature[]"}
        }
    });

    // actions def
    evt_abi.structs.emplace_back( struct_def {
        "newdomain", "", {
            {"name", "domain_name"},
            {"creator", "user_id"},
            {"issue", "permission_def"},
            {"transfer", "permission_def"},
            {"manage", "permission_def"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "issuetoken", "", {
            {"domain", "domain_name"},
            {"names", "token_name[]"},
            {"owner", "user_list"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "transfer", "", {
            {"domain", "domain_name"},
            {"name", "token_name"},
            {"to", "user_list"},
            {"memo", "string"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "destroytoken", "", {
            {"domain", "domain_name"},
            {"name", "token_name"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "newgroup", "", {
            {"name", "group_name"},
            {"group", "group_def"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "updategroup", "", {
            {"name", "group_name"},
            {"group", "group_def"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "updatedomain", "", {
            {"name", "domain_name"},
            {"issue", "permission_def?"},
            {"transfer", "permission_def?"},
            {"manage", "permission_def?"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "newfungible", "", {
            {"sym", "symbol"},
            {"creator", "user_id"},
            {"issue", "permission_def"},
            {"manage", "permission_def"},
            {"total_supply", "asset"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "updfungible", "", {
            {"sym", "symbol"},
            {"issue", "permission_def?"},
            {"manage", "permission_def?"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "issuefungible", "", {
            {"address", "public_key"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "transferft", "", {
            {"from", "public_key"},
            {"to", "public_key"},
            {"number", "asset"},
            {"memo", "string"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "addmeta", "", {
            {"key", "meta_key"},
            {"value", "meta_value"},
            {"creator", "user_id"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "newdelay", "", {
            {"name", "proposal_name"},
            {"proposer", "user_id"},
            {"trx", "transaction"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "canceldelay", "", {
            {"name", "proposal_name"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "approvedelay", "", {
            {"name", "proposal_name"},
            {"signatures", "signature[]"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "executedelay", "", {
           {"name", "proposal_name"}
        }
    });

    // abi_def fields
    evt_abi.structs.emplace_back( struct_def {
        "field_def", "", {
            {"name", "field_name"},
            {"type", "type_name"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "struct_def", "", {
            {"name", "type_name"},
            {"base", "type_name"},
            {"fields", "fields"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "type_def", "", {
            {"new_type_name", "type_name"},
            {"type", "type_name"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "action_def", "", {
           {"name", "action_name"},
           {"type", "type_name"}
        }
    });

    // blocks & transactions def
    evt_abi.structs.emplace_back( struct_def {
        "action", "", {
            {"name", "action_name"},
            {"domain", "domain_name"},
            {"key", "domain_key"},
            {"data", "bytes"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "transaction_header", "", {
            {"expiration", "time_point_sec"},
            {"region", "uint16"},
            {"ref_block_num", "uint16"},
            {"ref_block_prefix", "uint16"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "transaction", "transaction_header", {
            {"actions", "action[]"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
        "signed_transaction", "transaction", {
            {"signatures", "signature[]"}
        }
    });

    evt_abi.structs.emplace_back( struct_def {
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

    return evt_abi;
}

}}}  // namespace evt::chain::contracts