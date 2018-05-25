# set the chain id
if (NOT "${CHAIN_ID}" STREQUAL "")
   set(evt_CHAIN_ID "${CHAIN_ID}")
else()
   ## sha256("evt::chain version 1.0")
   set(evt_CHAIN_ID "1DAF81D021D658B7A55092411F8A856AF5EE2980C0832691B36FDE79B93F28F8")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/chain_id.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/chain_id.cpp)

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
             chain_id.cpp
             
             fork_database.cpp
             token_database.cpp

             apply_context.cpp
             controller.cpp

             contracts/evt_contract.cpp
             contracts/evt_contract_abi.cpp
             contracts/genesis_state.cpp
             contracts/abi_serializer.cpp
             contracts/group.cpp
             contracts/authorizer_ref.cpp

             ${HEADERS}
             )

target_link_libraries( evt_chain evt_utilities fc chainbase rocksdb )
target_include_directories( evt_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
