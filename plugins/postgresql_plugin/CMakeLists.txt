file(GLOB HEADERS "include/evt/postgresql_plugin/*.hpp")
add_library( postgresql_plugin
             postgresql_plugin.cpp
             evt_interpreter.cpp
             ${HEADERS} )

find_package(libbsonc REQUIRED)
message(STATUS "Found bsonc library: ${LIBBSONC_LIBRARIES}")

find_package(libmongoc REQUIRED)
message(STATUS "Found mongoc library: ${LIBMONGOC_LIBRARIES}")

if (LIBMONGOC_FOUND AND LIBBSONC_FOUND)

  # EVT has no direct dependencies on libmongoc but its static libraries
  # will need to be present at runtime for the C++ libraries we use:
  # libbsoncxx & libmongocxx (both from github.com/mongodb/mongo-cxx-driver)

  find_package(libbsoncxx REQUIRED)
  message(STATUS "Found bsoncxx headers: ${LIBBSONCXX_INCLUDE_DIR}")
  message(STATUS "Found bsoncxx library: ${LIBBSONCXX_LIBRARIES}")

  find_package(libmongocxx REQUIRED)
  message(STATUS "Found mongocxx headers: ${LIBMONGOCXX_INCLUDE_DIR}")
  message(STATUS "Found mongocxx library: ${LIBMONGOCXX_LIBRARIES}")
else()
  message("Could NOT find MongoDB. mongo_db_plugin with MongoDB support will not be included.")
endif()

target_include_directories(mongo_db_plugin
      PRIVATE ${LIBMONGOCXX_INCLUDE_DIR} ${LIBBSONCXX_INCLUDE_DIR}
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include"
      )

target_compile_definitions(mongo_db_plugin
PRIVATE ${LIBMONGOCXX_DEFINITIONS} ${LIBBSONCXX_DEFINITIONS}
)

target_link_libraries(mongo_db_plugin
      PUBLIC chain_plugin evt_chain appbase fc
      ${LIBBSONC_LIBRARIES} ${LIBMONGOC_LIBRARIES}
      ${LIBBSONCXX_LIBRARIES} ${LIBMONGOCXX_LIBRARIES}
      )