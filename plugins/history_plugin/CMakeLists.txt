file(GLOB HEADERS "include/evt/history_plugin/*.hpp")
add_library( history_plugin
             history_plugin.cpp
             evt_pg_query.cpp
             ${HEADERS} )

find_package(libbsoncxx QUIET REQUIRED)
find_package(libmongocxx QUIET REQUIRED)
find_package(libpq REQUIRED)

target_include_directories(history_plugin
    PRIVATE ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../mongo_db_plugin/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../http_plugin/include"
    PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(history_plugin
    PRIVATE ${LIBMONGOCXX_DEFINITIONS} ${LIBBSONCXX_DEFINITIONS}
)

target_link_libraries(history_plugin
    PUBLIC chain_plugin mongo_db_plugin postgres_plugin evt_chain appbase fc
    ${LIBBSONC_LIBRARIES} ${LIBMONGOC_LIBRARIES}
    ${LIBBSONCXX_LIBRARIES} ${LIBMONGOCXX_LIBRARIES}
    ${LIBPQ_LIBRARIES}
)