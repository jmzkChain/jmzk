add_executable( cypher_suites_tests test_cypher_suites.cpp )
target_link_libraries( cypher_suites_tests fc )

add_test(NAME cypher_suites_tests 
         COMMAND libraries/fc/test/crypto/cypher_suites_tests 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
