add_subdirectory( fc )
add_subdirectory( chainbase )
add_subdirectory( utilities )
add_subdirectory( appbase )

set( WITH_LZ4 ON CACHE BOOL "use LZ4" FORCE )
set( WITH_ZSTD ON CACHE BOOL "use Zstd" FORCE )
set( WITH_JEMALLOC OFF CACHE BOOL "use Jemalloc" FORCE )
add_subdirectory( rocksdb )

add_subdirectory( chain )
