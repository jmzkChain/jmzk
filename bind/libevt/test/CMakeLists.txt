add_executable( libjmzk_tests libjmzk_tests.cpp  )
target_link_libraries( libjmzk_tests libjmzk jmzk_chain_lite )
target_include_directories(libjmzk_tests 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../libraries/catch/include
        )

add_test(NAME libjmzk_tests
         COMMAND bind/libjmzk/test/libjmzk_tests
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
