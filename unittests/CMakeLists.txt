add_executable(evt_unittests
    main.cpp
    abi_tests.cpp
    contracts_tests.cpp
    tokendb_tests.cpp
    types_tests.cpp
    )

target_link_libraries( evt_unittests
                        PRIVATE ${Boost_LIBRARIES} appbase evt_chain evt_testing fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES} )


target_include_directories( evt_unittests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME evt_unittests
         COMMAND unittests/evt_unittests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
