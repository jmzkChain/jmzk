add_executable( libevt_tests libevt_tests.cpp  )
target_link_libraries( libevt_tests libevt evt_chain_lite ${Boost_LIBRARIES} )
target_include_directories( libevt_tests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME libevt_tests
         COMMAND bind/libevt/test/libevt_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
