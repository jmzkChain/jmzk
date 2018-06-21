file(GLOB HEADERS "include/evt/evt_plugin/*.hpp")
add_library( evt_plugin
        evt_plugin.cpp
        ${HEADERS} )

target_link_libraries( evt_plugin chain_plugin evt_chain appbase fc )
target_include_directories( evt_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
