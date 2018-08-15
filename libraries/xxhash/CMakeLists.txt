file(GLOB HEADERS "include/xxhash/*.h")

set(sources
   xxhash.c
   ${HEADERS})

add_library(xxhash ${sources})
target_include_directories(xxhash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/xxhash")
