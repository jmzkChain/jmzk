add_executable( json_tests json_tests.cpp  )
target_link_libraries( json_tests fc ${Boost_LIBRARIES} )
target_include_directories( json_tests PUBLIC ${Boost_INCLUDE_DIR} )

add_test(NAME json_tests
         COMMAND libraries/fc/test/io/json_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
