add_executable(evt_unittests
    main.cpp
    abi_tests.cpp
    # tokendb_tests.cpp
    types_tests.cpp
    # snapshot_tests.cpp
    execution_tests.cpp
    
    contracts/token_tests.cpp
    contracts/group_tests.cpp
    contracts/fungible_tests.cpp
    contracts/suspend_tests.cpp
    contracts/lock_tests.cpp
    contracts/bonus_tests.cpp
    contracts/utils_tests.cpp
    )

target_link_libraries(evt_unittests
        PRIVATE appbase evt_chain evt_testing fc catch ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES})

add_test(NAME evt_unittests
         COMMAND unittests/evt_unittests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
