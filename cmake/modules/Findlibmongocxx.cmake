# - Find libmongocxx library
# Find the native libmongocxx includes and library
#
# LIBMONGOCXX_INCLUDE_DIR - where to find libmongocxx, etc.
# LIBMONGOCXX_LIBRARIES - List of libraries when using libmongocxx.
# LIBMONGOCXX_FOUND - True if libmongocxx found.

find_path(LIBMONGOCXX_INCLUDE_DIR
  NAMES mongocxx/v_noabi/mongocxx/document.hpp
  HINTS ${BSONCXX_ROOT_DIR}/include)

find_library(LIBMONGOCXX_LIBRARIES
  NAMES libmongocxx.so
  HINTS ${BSONCXX_ROOT_DIR}/lib)

string(CONCAT LIBMONGOCXX_INCLUDE_DIR ${LIBMONGOCXX_INCLUDE_DIR} "/mongocxx/v_noabi")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libmongocxx DEFAULT_MSG LIBMONGOCXX_LIBRARIES LIBMONGOCXX_INCLUDE_DIR)

mark_as_advanced(
  LIBMONGOCXX_LIBRARIES
  LIBMONGOCXX_INCLUDE_DIR)
