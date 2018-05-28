# - Find libbsoncxx library
# Find the native libbsoncxx includes and library
#
# LIBBSONCXX_INCLUDE_DIR - where to find libbsoncxx, etc.
# LIBBSONCXX_LIBRARIES - List of libraries when using libbsoncxx.
# LIBBSONCXX_FOUND - True if libbsoncxx found.

find_path(LIBBSONCXX_INCLUDE_DIR
  NAMES bsoncxx/v_noabi/bsoncxx/json.hpp
  HINTS ${BSONCXX_ROOT_DIR}/include)

find_library(LIBBSONCXX_LIBRARIES
  NAMES libbsoncxx.so
  HINTS ${BSONCXX_ROOT_DIR}/lib)

string(CONCAT LIBBSONCXX_INCLUDE_DIR ${LIBBSONCXX_INCLUDE_DIR} "/bsoncxx/v_noabi")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libbsoncxx DEFAULT_MSG LIBBSONCXX_LIBRARIES LIBBSONCXX_INCLUDE_DIR)

mark_as_advanced(
  LIBBSONCXX_LIBRARIES
  LIBBSONCXX_INCLUDE_DIR)
