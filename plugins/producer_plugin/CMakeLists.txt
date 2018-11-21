file(GLOB HEADERS "include/evt/producer_plugin/*.hpp")

add_library( producer_plugin
             producer_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( producer_plugin chain_plugin http_client_plugin appbase evt_chain evt_utilities net_plugin )
target_include_directories( producer_plugin
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../postgres_plugin/include"
    )
