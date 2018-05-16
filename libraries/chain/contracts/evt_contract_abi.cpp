/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/types.hpp>

namespace evt { namespace chain { namespace contracts { 

abi_def
evt_contract_abi() {
   abi_def evt_abi;
   evt_abi.types.push_back( type_def{"user_id","public_key"} );
   evt_abi.types.push_back( type_def{"user_list","public_key[]"} );
   evt_abi.types.push_back( type_def{"group_key","public_key"} );
   evt_abi.types.push_back( type_def{"weight_type","uint16"} );
   evt_abi.types.push_back( type_def{"fields","field[]"} );
   evt_abi.types.push_back( type_def{"time_point_sec","time"} );
   evt_abi.types.push_back( type_def{"permission_name","name"} );
   evt_abi.types.push_back( type_def{"action_name","name"} );
   evt_abi.types.push_back( type_def{"domain_name","name128"} );
   evt_abi.types.push_back( type_def{"group_name","name128"} );
   evt_abi.types.push_back( type_def{"token_name","name128"} );
   evt_abi.types.push_back( type_def{"account_name","name128"} );
   evt_abi.types.push_back( type_def{"domain_key","uint128"} );
   evt_abi.types.push_back( type_def{"balance_type","asset"} );
   evt_abi.types.push_back( type_def{"group_def","group"} );

   evt_abi.actions.push_back( action_def{name("newdomain"), "newdomain"} );
   evt_abi.actions.push_back( action_def{name("issuetoken"), "issuetoken"} );
   evt_abi.actions.push_back( action_def{name("transfer"), "transfer"} );
   evt_abi.actions.push_back( action_def{name("newgroup"), "newgroup"} );
   evt_abi.actions.push_back( action_def{name("updategroup"), "updategroup"} );
   evt_abi.actions.push_back( action_def{name("updatedomain"), "updatedomain"} );
   evt_abi.actions.push_back( action_def{name("newaccount"), "newaccount"} );
   evt_abi.actions.push_back( action_def{name("updateowner"), "updateowner"} );
   evt_abi.actions.push_back( action_def{name("transferevt"), "transferevt"} );

   // actions def
   evt_abi.structs.emplace_back( struct_def {
      "token_def", "", {
         {"domain", "domain_name"},
         {"name", "token_name"},
         {"owner", "user_list"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "key_weight", "", {
         {"key", "public_key"},
         {"weight", "weight_type"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "group_def", "", {
         {"name", "group_name"},
         {"key", "group_key"},
         {"threshold", "uint32"},
         {"keys", "key_weight[]"}
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
         {"issuer", "user_id"},
         {"issue_time", "time_point_sec"},
         {"issue", "permission_def"},
         {"transfer", "permission_def"},
         {"manage", "permission_def"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "account_def", "", {
         {"name", "account_name"},
         {"creator", "account_name"},
         {"balance", "balance_type"},
         {"frozen_balance", "balance_type"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "newdomain", "", {
         {"name", "domain_name"},
         {"issuer", "user_id"},
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
         {"to", "user_list"}
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
      "newaccount", "", {
         {"name", "account_name"},
         {"owner", "user_list"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "updateowner", "", {
         {"name", "account_name"},
         {"owner", "user_list"}
      }
   });

   evt_abi.structs.emplace_back( struct_def {
      "transferevt", "", {
         {"from", "account_name"},
         {"to", "account_name"},
         {"amount", "balance_type"}
      }
   });

   // abi_def fields
   evt_abi.structs.emplace_back( struct_def {
      "field", "", {
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