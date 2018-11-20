file(GLOB HEADERS "include/evt/history_plugin/*.hpp")
add_library( history_plugin
             history_plugin.cpp
             evt_pg_query.cpp
             ${HEADERS} )

find_package(libpq REQUIRED)

target_include_directories(history_plugin
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../http_plugin/include"
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(history_plugin
    PUBLIC chain_plugin postgres_plugin evt_chain appbase fc fmt-header-only
    PUBLIC ${LIBPQ_LIBRARIES}
)

target_compile_definitions(history_plugin PUBLIC FMT_STRING_ALIAS=1)
