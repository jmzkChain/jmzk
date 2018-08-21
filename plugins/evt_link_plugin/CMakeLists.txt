file(GLOB HEADERS "include/evt/evt_link_plugin/*.hpp")
add_library( evt_link_plugin
             evt_link_plugin.cpp
             ${HEADERS} )

target_link_libraries( evt_link_plugin chain_plugin http_plugin evt_chain appbase )
target_include_directories( evt_link_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
