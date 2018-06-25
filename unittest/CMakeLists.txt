add_executable( evt_unittests abi_tests.cpp  )

target_link_libraries( evt_unittests
                        PRIVATE ${Boost_LIBRARIES} appbase evt_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )


target_include_directories( evt_unittests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME evt_unittests
         COMMAND unittest/evt_unittests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
