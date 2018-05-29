# - Find libbsonc library
# Find the native libbsonc includes and library
#
# LIBBSONC_INCLUDE_DIR - where to find libbsonc, etc.
# LIBBSONC_LIBRARIES - List of libraries when using libbsonc.
# LIBBSONC_FOUND - True if libbsonc found.

find_path(LIBBSONC_INCLUDE_DIR
  NAMES bsoncxx/v_noabi/bsoncxx/json.hpp
  HINTS ${BSONC_ROOT_DIR}/include)

find_library(LIBBSONC_LIBRARIES
  NAMES bson-1.0
  HINTS ${BSONC_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libbsonc DEFAULT_MSG LIBBSONC_LIBRARIES LIBBSONC_INCLUDE_DIR)

mark_as_advanced(
  LIBBSONC_LIBRARIES
  LIBBSONC_INCLUDE_DIR)
