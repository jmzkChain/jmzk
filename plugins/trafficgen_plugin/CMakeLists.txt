file(GLOB HEADERS "include/jmzk/trafficgen_plugin/*.hpp")
add_library( trafficgen_plugin
             trafficgen_plugin.cpp
             ${HEADERS} )

target_link_libraries( trafficgen_plugin chain_plugin appbase )
target_include_directories( trafficgen_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
