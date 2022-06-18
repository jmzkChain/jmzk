file(GLOB HEADERS "include/jmzk/jmzk_plugin/*.hpp")
add_library( jmzk_plugin
        jmzk_plugin.cpp
        ${HEADERS} )

target_link_libraries( jmzk_plugin chain_plugin jmzk_chain appbase fc )
target_include_directories( jmzk_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
