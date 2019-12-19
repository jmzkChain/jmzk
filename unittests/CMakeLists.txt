add_executable(evt_unittests
    main.cpp
    abi_tests.cpp
    types_tests.cpp
    crypto_tests.cpp

    tokendb/basic_tests.cpp
    tokendb/runtime_tests.cpp
    tokendb/persist_tests.cpp
    tokendb/cache_tests.cpp
    
    snapshot_tests.cpp
    luajit_tests.cpp
    
    contracts/nft_tests.cpp
    contracts/group_tests.cpp
    contracts/ft_tests.cpp
    contracts/suspend_tests.cpp
    contracts/lock_tests.cpp
    contracts/bonus_tests.cpp
    contracts/utils_tests.cpp
    contracts/evtlink_tests.cpp
    contracts/staking_tests.cpp
    contracts/multi_actions_tests.cpp
    )
set_target_properties(evt_unittests PROPERTIES ENABLE_EXPORTS TRUE)

find_package(Jemalloc QUIET)
if(JEMALLOC_FOUND)
    message(STATUS "Found jemalloc; compiling evt_unittests with jemalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS jemalloc)
endif()

if(UNIX)
    if(APPLE)
        set_target_properties(evt_unittests PROPERTIES LINK_FLAGS "${LUAJIT_LINK_FLAGS}")
    endif()
endif()

target_link_libraries(evt_unittests PRIVATE
    appbase evt_chain evt_testing fc catch ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES})

add_test(NAME evt_unittests
         COMMAND unittests/evt_unittests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
