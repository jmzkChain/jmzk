file(GLOB HEADERS "include/evt/history_api_plugin/*.hpp")
add_library( history_api_plugin
             history_api_plugin.cpp
             ${HEADERS} )

find_package(libbsoncxx QUIET REQUIRED)
find_package(libmongocxx QUIET REQUIRED)

target_link_libraries( history_api_plugin history_plugin http_plugin appbase )
target_include_directories( history_api_plugin
    PRIVATE ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
