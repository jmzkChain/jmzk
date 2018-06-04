configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis_state_root_key.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp)

file(GLOB HEADERS "include/evt/chain/*.hpp" "include/evt/chain/contracts/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( evt_chain
             asset.cpp
             merkle.cpp
             name.cpp
             transaction.cpp
             transaction_context.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             block_log.cpp

             chain_config.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp
             
             fork_database.cpp
             token_database.cpp

             apply_context.cpp
             controller.cpp

             contracts/evt_contract.cpp
             contracts/evt_contract_abi.cpp
             contracts/abi_serializer.cpp
             contracts/group.cpp
             contracts/authorizer_ref.cpp

             ${HEADERS}
             )

add_library( evt_chain_lite SHARED
             asset.cpp
             name.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp
             contracts/abi_serializer.cpp
             contracts/group.cpp
             contracts/authorizer_ref.cpp

             ${HEADERS})

target_link_libraries( evt_chain evt_utilities fc chainbase rocksdb )
target_include_directories( evt_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            )

target_link_libraries( evt_chain_lite fc_lite )
target_include_directories( evt_chain_lite
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chainbase/include" 
                            )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
