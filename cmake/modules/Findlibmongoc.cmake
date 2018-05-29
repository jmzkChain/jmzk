# - Find libmongoc library
# Find the native libmongoc includes and library
#
# LIBMONGOC_INCLUDE_DIR - where to find libmongoc, etc.
# LIBMONGOC_LIBRARIES - List of libraries when using libmongoc.
# LIBMONGOC_FOUND - True if libmongoc found.

find_path(LIBMONGOC_INCLUDE_DIR
  NAMES libmongoc-1.0/mongoc.h
  HINTS ${MONGOC_ROOT_DIR}/include)

find_library(LIBMONGOC_LIBRARIES
  NAMES mongoc-1.0
  HINTS ${MONGOC_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libmongoc DEFAULT_MSG LIBMONGOC_LIBRARIES LIBMONGOC_INCLUDE_DIR)

mark_as_advanced(
  LIBMONGOC_LIBRARIES
  LIBMONGOC_INCLUDE_DIR)
