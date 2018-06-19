add_executable( unit_tests abi_tests.cpp  )

target_link_libraries( unit_tests
                        PRIVATE ${Boost_LIBRARIES} appbase evt_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )


target_include_directories( unit_tests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME unit_tests
         COMMAND unittest/unit_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})