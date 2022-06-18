file(GLOB HEADERS "include/jmzk/jmzk_api_plugin/*.hpp")
add_library( jmzk_api_plugin
             jmzk_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( jmzk_api_plugin jmzk_plugin http_plugin appbase )
target_include_directories( jmzk_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
