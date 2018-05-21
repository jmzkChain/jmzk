add_subdirectory( fc )
add_subdirectory( chainbase )
add_subdirectory( utilities )
add_subdirectory( appbase )

set( WITH_GFLAGS OFF CACHE BOOL "use GFlags" FORCE )
set( WITH_LZ4 ON CACHE BOOL "use LZ4" FORCE )
set( WITH_ZSTD ON CACHE BOOL "use Zstd" FORCE )
set( WITH_JEMALLOC OFF CACHE BOOL "use Jemalloc" FORCE )
set( WITH_TESTS OFF CACHE BOOL "with Tests" FORCE )
set( WITH_TOOLS OFF CACHE BOOL "with Tools" FORCE )
add_subdirectory( rocksdb EXCLUDE_FROM_ALL )

add_subdirectory( chain )
