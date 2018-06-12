# - Find benchamrk library
# Find the native benchamrk includes and library
#
# BENCHMARK_INCLUDE_DIR - where to find benchamrk.h, etc.
# BENCHMARK_LIBRARIES - List of libraries when using benchamrk.
# BENCHMARK_FOUND - True if benchamrk found.

find_path(BENCHMARK_INCLUDE_DIR
  NAMES benchmark/benchmark.h
  HINTS ${BENCHMARK_ROOT_DIR}/include)

find_library(BENCHMARK_LIBRARIES
  NAMES libbenchmark.a
  HINTS ${BENCHMARK_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(benchamrk DEFAULT_MSG BENCHMARK_LIBRARIES BENCHMARK_INCLUDE_DIR)

mark_as_advanced(
  BENCHMARK_LIBRARIES
  BENCHMARK_INCLUDE_DIR)
