file(GLOB HEADERS "include/evt/testing/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( evt_testing
             tester.cpp
             tester_network.cpp
             ${HEADERS}
        )

target_link_libraries( evt_testing evt_chain eos_utilities fc chainbase )
target_include_directories( evt_testing
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

set_target_properties( evt_testing PROPERTIES PUBLIC_HEADER "${HEADERS}" )
install( TARGETS evt_testing
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/evt/testing OPTIONAL
)
install_directory_permissions( DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/evt/testing )

