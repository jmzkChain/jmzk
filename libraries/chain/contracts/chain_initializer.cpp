/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/contracts/evt_contract.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/permission_object.hpp>

#include <fc/io/json.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace eosio { namespace chain { namespace contracts {

time_point chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

chain_config chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

producer_schedule_type  chain_initializer::get_chain_start_producers() {
   producer_schedule_type result;
   result.producers.push_back( {config::system_account_name, genesis.initial_key} );
   return result;
}

void chain_initializer::register_types(chain_controller& chain, chainbase::database& db) {

#define SET_APP_HANDLER( contract, scope, action, nspace ) \
   chain._set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(contracts::apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )

    SET_APP_HANDLER( eosio, eosio, newdomain, eosio );
    SET_APP_HANDLER( eosio, eosio, issuetoken, eosio );
    SET_APP_HANDLER( eosio, eosio, transfer, eosio );
    SET_APP_HANDLER( eosio, eosio, updategroup, eosio );
    SET_APP_HANDLER( eosio, eosio, updatedomain, eosio );
}


abi_def chain_initializer::eos_contract_abi(const abi_def& eosio_system_abi)
{
   abi_def eos_abi(eosio_system_abi);
   eos_abi.types.push_back( type_def{"user_id","public_key"} );
   eos_abi.types.push_back( type_def{"user_list","public_key[]"} );
   eos_abi.types.push_back( type_def{"group_key","public_key"} );
   eos_abi.types.push_back( type_def{"group_id","uint128_t"} );
   eos_abi.types.push_back( type_def{"permission_name","name"} );
   eos_abi.types.push_back( type_def{"weight_type","uint16"} );
   eos_abi.types.push_back( type_def{"fields","field[]"} );
   eos_abi.types.push_back( type_def{"time_point_sec","time"} );

   eos_abi.actions.push_back( action_def{name("newdomain"), "newdomain"} );
   eos_abi.actions.push_back( action_def{name("issuetoken"), "issuetoken"} );
   eos_abi.actions.push_back( action_def{name("transfer"), "transfer"} );
   eos_abi.actions.push_back( action_def{name("updategroup"), "updategroup"} );  
   eos_abi.actions.push_back( action_def{name("updatedomain"), "updatedomain"} );  

   // ACTION PAYLOADS

   eos_abi.structs.emplace_back( struct_def {
      "newdomain", "", {
         {"name", "domain_name"},
         {"issuer", "user_id"},
         {"issue", "permission_def"},
         {"transfer", "permission_def"},
         {"manage", "permission_def"},
         {"groups", "group_def[]"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "issuetoken", "", {
         {"domain", "domain_name"},
         {"names", "token_name[]"},
         {"owner", "user_list"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transfer", "", {
         {"domain", "domain_name"},
         {"token_name", "name"},
         {"to", "user_list"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "updategroup", "", {
         {"id", "group_id"},
         {"threshold", "uint32"},
         {"keys", "key_weight[]"},
         {"requirement", "permission_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "updatedomain", "", {
         {"name", "domain_name"},
         {"issue", "permission_def"},
         {"transfer", "permission_def"},
         {"manage", "permission_def"},
         {"groups", "group_def[]"},
      }
   });

   // DATABASE RECORDS

   eos_abi.structs.emplace_back( struct_def {
      "pending_recovery", "", {
         {"account",    "name"},
         {"request_id", "uint32"},
         {"update",     "updateauth"},
         {"memo",       "string"}
      }
   });

   // abi_def fields

   eos_abi.structs.emplace_back( struct_def {
      "field", "", {
         {"name", "field_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "struct_def", "", {
         {"name", "type_name"},
         {"base", "type_name"},
         {"fields", "fields"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "token_def", "", {
         {"domain", "domain_name"},
         {"name", "token_name"},
         {"owner", "user_list"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "group_def", "", {
         {"id", "group_id"},
         {"key", "group_key"},
         {"threshold", "uint32"},
         {"keys", "key_weight[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "group_weight", "", {
         {"id", "group_id"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_def", "", {
         {"name", "permission_name"},
         {"threshold", "uint32"},
         {"groups", "group_weight[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "domain_def", "", {
         {"name", "domain_name"},
         {"issuer", "user_id"},
         {"issue_time", "time_point_sec"},
         {"issue", "permission_def"},
         {"transfer", "permission_def"},
         {"manage", "permission_def"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level", "", {
         {"actor", "account_name"},
         {"permission", "permission_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action", "", {
         {"account", "account_name"},
         {"name", "action_name"},
         {"domain", "domain_name"},
         {"key", "domain_key"},
         {"data", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level_weight", "", {
         {"permission", "permission_level"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction_header", "", {
         {"expiration", "time_point_sec"},
         {"region", "uint16"},
         {"ref_block_num", "uint16"},
         {"ref_block_prefix", "uint16"},
         {"packed_bandwidth_words", "uint16"},
         {"context_free_cpu_bandwidth", "uint16"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction", "transaction_header", {
         {"actions", "action[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "signed_transaction", "transaction", {
         {"signatures", "signature[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "key_weight", "", {
         {"key", "public_key"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "authority", "", {
         {"threshold", "uint32"},
         {"keys", "key_weight[]"},
         {"accounts", "permission_level_weight[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "type_def", "", {
         {"new_type_name", "type_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action_def", "", {
         {"name", "action_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
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

   return eos_abi;
}

void chain_initializer::prepare_database( chain_controller& chain,
                                                         chainbase::database& db) {
   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto create_native_account = [this, &db](account_name name) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
         a.privileged = true;

         if( name == config::system_account_name ) {
            a.set_abi(abi_def());
         }
      });
      const auto& owner = db.create<permission_object>([&](permission_object& p) {
         p.owner = name;
         p.name = "owner";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = genesis.initial_key, .weight = 1 } );
      });
      db.create<permission_object>([&](permission_object& p) {
         p.owner = name;
         p.parent = owner.id;
         p.name = "active";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = genesis.initial_key, .weight = 1 } );
      });
      db.create<bandwidth_usage_object>([&](auto& sb) { 
         sb.owner = name;      
         sb.net_weight  = -1;
         sb.cpu_weight  = -1;
         sb.db_reserved_capacity = -1;
      });

      db.create<producer_object>( [&]( auto& pro ) {
         pro.owner = config::system_account_name;
         pro.signing_key = genesis.initial_key;
      });
   };
   create_native_account(config::system_account_name);

   // Create special accounts
   auto create_special_account = [this, &db](account_name name, const auto& owner, const auto& active) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      const auto& owner_permission = db.create<permission_object>([&owner, &name](permission_object& p) {
         p.name = config::owner_name;
         p.parent = 0;
         p.owner = name;
         p.auth = move(owner);
      });
      db.create<permission_object>([&active, &owner_permission](permission_object& p) {
         p.name = config::active_name;
         p.parent = owner_permission.id;
         p.owner = owner_permission.owner;
         p.auth = move(active);
      });
   };

   auto empty_authority = authority(0, {}, {});
   auto active_producers_authority = authority(config::producers_authority_threshold, {}, {});
   active_producers_authority.accounts.push_back({{config::system_account_name, config::active_name}, 1});

   create_special_account(config::nobody_account_name, empty_authority, empty_authority);
   create_special_account(config::producers_account_name, empty_authority, active_producers_authority);
}

void chain_initializer::prepare_tokendb(chain::chain_controller& chain, evt::chain::tokendb& tokendb) {
    if(!tokendb.exists_domain("domain")) {
        domain_def ddomain;
        ddomain.issuer = genesis.initial_key;
        auto r = tokendb.add_domain(ddomain);
        FC_ASSERT(r == 0, "Add `domain` domain failed");
    }
    if(!tokendb.exists_domain("group")) {
        domain_def gdomain;
        gdomain.issuer = genesis.initial_key;
        auto r = tokendb.add_domain(gdomain);
        FC_ASSERT(r == 0, "Add `group` domain failed");
    }
}

} } } // namespace eosio::chain::contracts
