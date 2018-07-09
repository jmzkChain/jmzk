# - Find breakpad
# Find the breakpad library and includes
#
# BREAKPAD_INCLUDE_DIR - where to find breakpad.h, etc.
# BREAKPAD_LIBRARIES - List of libraries when using breakpad.
# BREAKPAD_FOUND - True if breakpad found.

find_path(BREAKPAD_INCLUDE_DIR
  NAMES breakpad/client
  HINTS ${BREAKPAD_ROOT_DIR}/include)

find_library(BREAKPAD_LIBRARIES
  NAMES libbreakpad_client.a
  HINTS ${BREAKPAD_ROOT_DIR}/lib)

string(CONCAT BREAKPAD_INCLUDE_DIR ${BREAKPAD_INCLUDE_DIR} "/breakpad")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(breakpad DEFAULT_MSG BREAKPAD_LIBRARIES BREAKPAD_INCLUDE_DIR)

mark_as_advanced(
  BREAKPAD_LIBRARIES
  BREAKPAD_INCLUDE_DIR)
