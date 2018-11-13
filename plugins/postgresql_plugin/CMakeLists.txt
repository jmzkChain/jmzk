file(GLOB HEADERS "include/evt/postgresql_plugin/*.hpp")
add_library( postgresql_plugin
             evt_pg.cpp
             ${HEADERS} )

find_package(libpq REQUIRED)

target_include_directories(postgresql_plugin
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${LIBPQ_INCLUDE_DIR}"
      )

target_link_libraries(postgresql_plugin
      PUBLIC chain_plugin evt_chain appbase fc fmt-header-only ${LIBPQ_LIBRARIES}
      )