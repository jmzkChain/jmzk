file(GLOB HEADERS "include/evt/postgresql_plugin/*.hpp")
add_library( postgresql_plugin
             evt_pg.cpp
             ${HEADERS} )

target_include_directories(postgresql_plugin
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include"
      )

target_link_libraries(postgresql_plugin
      PUBLIC chain_plugin evt_chain appbase fc fmt-header-only
      )