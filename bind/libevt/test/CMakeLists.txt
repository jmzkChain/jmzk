add_executable( libevt_tests libevt_tests.cpp  )
target_link_libraries( libevt_tests libevt evt_chain_lite )
target_include_directories(libevt_tests 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../libraries/catch/include
        )

add_test(NAME libevt_tests
         COMMAND bind/libevt/test/libevt_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
