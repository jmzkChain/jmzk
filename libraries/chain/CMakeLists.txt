file(GLOB HEADERS "include/evt/chain/*.hpp" "include/evt/chain/contracts/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( evt_chain
             chain_config.cpp
             merkle.cpp
             name.cpp
             transaction.cpp
             block.cpp
             apply_context.cpp

             fork_database.cpp
             get_config.cpp
             block_log.cpp
             block_trace.cpp
             tokendb.cpp
             asset.cpp

             global_property_object.cpp
             chain_controller.cpp

             contracts/evt_contract.cpp
             contracts/chain_initializer.cpp
             contracts/genesis_state.cpp
             contracts/abi_serializer.cpp
             contracts/group_id.cpp

             ${HEADERS}
             transaction_metadata.cpp)

target_link_libraries( evt_chain evt_utilities fc chainbase softfloat rocksdb )
target_include_directories( evt_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

add_subdirectory(test)

#add_executable( test test.cpp )
#target_link_libraries( test evt_chain ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )


