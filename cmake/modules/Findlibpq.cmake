# - Find libpq
# Find the libpq compression library and includes
#
# LIBPQ_INCLUDE_DIR - where to find libpq.h, etc.
# LIBPQ_LIBRARIES - List of libraries when using libpq.
# LIBPQ_FOUND - True if libpq found.

find_path(LIBPQ_INCLUDE_DIR
  NAMES libpq-fe.h
  HINTS ${LIBPQ_ROOT_DIR}/include
  HINTS /usr/local/opt/libpq/include)

find_library(LIBPQ_LIBRARIES
  NAMES libpq.so libpq.dylib
  HINTS ${LIBPQ_ROOT_DIR}/lib
  HINTS /usr/local/opt/libpq/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libpq DEFAULT_MSG LIBPQ_LIBRARIES LIBPQ_INCLUDE_DIR)

mark_as_advanced(
  LIBPQ_LIBRARIES
  LIBPQ_INCLUDE_DIR)
