add_library(sss STATIC
    hazmat.c
    randombytes.c)
target_include_directories(sss PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
