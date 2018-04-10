file(GLOB HEADERS "include/evt/evt_api_plugin/*.hpp")
add_library( evt_api_plugin
             evt_api_plugin.cpp
             ${HEADERS} )

target_link_libraries( evt_api_plugin evt_plugin http_plugin appbase )
target_include_directories( evt_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
