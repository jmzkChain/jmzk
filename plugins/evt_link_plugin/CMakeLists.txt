file(GLOB HEADERS "include/jmzk/jmzk_link_plugin/*.hpp")
add_library( jmzk_link_plugin
             jmzk_link_plugin.cpp
             ${HEADERS} )

target_link_libraries( jmzk_link_plugin chain_plugin http_plugin jmzk_chain appbase )
target_include_directories( jmzk_link_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
