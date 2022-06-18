file(GLOB HEADERS "include/jmzk/postgres_plugin/*.hpp")
add_library( postgres_plugin
             jmzk_pg.cpp
             postgres_plugin.cpp
             ${HEADERS} )

find_package(libpq REQUIRED)

target_include_directories(postgres_plugin
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include" "${LIBPQ_INCLUDE_DIR}"
      )

target_link_libraries(postgres_plugin
      PUBLIC chain_plugin jmzk_chain appbase fc fmt-header-only ${LIBPQ_LIBRARIES}
      )

target_compile_definitions(postgres_plugin PUBLIC FMT_STRING_ALIAS=1)
