# - Find luajit
# Find the luajit library and includes
#
# LUAJIT_INCLUDE_DIR - where to find luajit.h, etc.
# LUAJIT_LIBRARIES - List of libraries when using luajit.
# LUAJIT_FOUND - True if luajit found.

find_path(LUAJIT_INCLUDE_DIR
  NAMES luajit-2.1/luajit.h
  HINTS ${LUAJIT_ROOT_DIR}/include)

find_library(LUAJIT_LIBRARIES
  NAMES libluajit-5.1.so
  HINTS ${LUAJIT_ROOT_DIR}/lib)

string(CONCAT LUAJIT_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR} "/luajit-2.1")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(luajit DEFAULT_MSG LUAJIT_LIBRARIES LUAJIT_INCLUDE_DIR)

mark_as_advanced(
  LUAJIT_LIBRARIES
  LUAJIT_INCLUDE_DIR)
