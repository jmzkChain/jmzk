file(GLOB UNIT_TESTS "*.cpp")
add_executable( libevt_tests ${UNIT_TESTS}  )
target_link_libraries( libevt_tests libevt ${Boost_LIBRARIES} )
target_include_directories( libevt_tests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME libevt_tests
         COMMAND bind/libevt/test/libevt_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})