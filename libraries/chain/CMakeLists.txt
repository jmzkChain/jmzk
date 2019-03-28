configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis_state_root_key.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp)

file(GLOB HEADERS "include/evt/chain/*.hpp" "include/evt/chain/contracts/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( evt_chain
             address.cpp
             asset.cpp
             percent_slim.cpp
             merkle.cpp
             name.cpp
             name128.cpp
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
             token_database_snapshot.cpp
             snapshot.cpp

             apply_context.cpp
             execution_context.cpp
             controller.cpp

             contracts/authorizer_ref.cpp
             contracts/group.cpp
             contracts/evt_link.cpp
             contracts/evt_org.cpp
             contracts/evt_contract_abi.cpp
             contracts/abi_serializer.cpp

             ${HEADERS}
             )

add_library( evt_chain_lite SHARED
             address.cpp
             asset.cpp
             name.cpp
             name128.cpp
             transaction.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp
             contracts/abi_serializer.cpp
             contracts/group.cpp
             contracts/authorizer_ref.cpp
             contracts/evt_link.cpp
             contracts/evt_contract_abi.cpp

             ${HEADERS}
             )

find_package(LLVM REQUIRED)

target_link_libraries( evt_chain evt_utilities fc chainbase rocksdb fmt-header-only sparsehash ${LLVM_LIBRARIES} )
target_include_directories( evt_chain
                            PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_BINARY_DIR}/include"
                            "${LLVM_INCLUDE_DIR}"
                            "${LLVM_C_INCLUDE_DIR}"
                            )

target_link_libraries( evt_chain_lite fc_lite fmt-header-only sparsehash ${LLVM_LIBRARIES} )
target_include_directories( evt_chain_lite
                            PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_BINARY_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../chainbase/include" 
                            "${CMAKE_CURRENT_SOURCE_DIR}/../utilities/include"
                            "${LLVM_INCLUDE_DIR}"
                            "${LLVM_C_INCLUDE_DIR}"
                            )

set_target_properties( evt_chain PROPERTIES PUBLIC_HEADER "${HEADERS}" )

target_compile_definitions(evt_chain PUBLIC FMT_STRING_ALIAS=1)

# find_package(TBB)

if(ENABLE_MAINNET_BUILD)
    target_compile_definitions(evt_chain PUBLIC MAINNET_BUILD)
endif()

install( TARGETS evt_chain
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/evt/chain OPTIONAL
)
install_directory_permissions( DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/evt/chain )

install( TARGETS evt_chain_lite
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
)
