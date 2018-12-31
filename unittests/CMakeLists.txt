add_executable(evt_unittests
    main.cpp
    abi_tests.cpp
    contracts_tests.cpp
    tokendb_tests.cpp
    types_tests.cpp
    snapshot_tests.cpp
    execution_tests.cpp
    )

target_link_libraries(evt_unittests
        PRIVATE appbase evt_chain evt_testing fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES})


target_include_directories(evt_unittests 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libraries/catch/include)

add_test(NAME evt_unittests
         COMMAND unittests/evt_unittests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
