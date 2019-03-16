# - Find LLVM
# Find the LLVM includes
#
# LLVM_INCLUDE_DIR - where to find InitializePasses.h, etc.
# LLVM_FOUND - True if libpq found.

find_path(LLVM_INCLUDE_DIR
  NAMES llvm/InitializePasses.h
  HINTS ${LLVM_ROOT_DIR}
  HINTS /usr/include/llvm-7
  HINTS /usr/include/llvm-8
  HINTS /usr/local/include/llvm-7
  HINTS /usr/local/include/llvm-8)

find_path(LLVM_C_INCLUDE_DIR
  NAMES llvm-c/DebugInfo.h
  HINTS ${LLVM_C_ROOT_DIR}
  HINTS /usr/include/llvm-c-7
  HINTS /usr/include/llvm-c-8
  HINTS /usr/local/include/llvm-c-7
  HINTS /usr/local/include/llvm-c-8)

find_library(LLVM_LIBRARIES
  NAMES lib/libLLVM.so
  HINTS /usr/lib/llvm-7
  HINTS /usr/lib/llvm-8)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLVM DEFAULT_MSG LLVM_INCLUDE_DIR LLVM_C_INCLUDE_DIR LLVM_LIBRARIES)

mark_as_advanced(
  LLVM_INCLUDE_DIR
  LLVM_C_INCLUDE_DIR
  LLVM_LIBRARIES)
