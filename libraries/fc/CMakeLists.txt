# Setup module path to make visible used CMake extensions
#INCLUDE(GetPrerequisites)
#INCLUDE(VersionMacros)

set(DEFAULT_HEADER_INSTALL_DIR usr/include/${target})
set(DEFAULT_LIBRARY_INSTALL_DIR usr/lib)
set(DEFAULT_EXECUTABLE_INSTALL_DIR usr/bin)
set(CMAKE_DEBUG_POSTFIX _debug)
set(BUILD_SHARED_LIBS NO)
set(ECC_IMPL secp256k1 CACHE STRING "secp256k1 or openssl or mixed")

set(platformBitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(platformBitness 64)
endif()

set(ORIGINAL_LIB_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

find_package(Secp256k1 REQUIRED)
find_package(GMP REQUIRED)

if(ECC_IMPL STREQUAL openssl)
    set(ECC_REST src/crypto/elliptic_impl_pub.cpp)
else(ECC_IMPL STREQUAL openssl)
    set(ECC_LIB ${Secp256k1_LIBRARY} ${GMP_LIBRARIES})
    set(ECC_SHARED_LIB ${Secp256k1_SHARED_LIBRARY} ${GMP_SHARED_LIBRARIES})
    if(ECC_IMPL STREQUAL mixed)
        set(ECC_REST src/crypto/elliptic_impl_priv.cpp src/crypto/elliptic_impl_pub.cpp)
    else(ECC_IMPL STREQUAL mixed)
        set(ECC_REST src/crypto/elliptic_impl_priv.cpp)
    endif(ECC_IMPL STREQUAL mixed)
endif(ECC_IMPL STREQUAL openssl)

message(STATUS "Configuring fc to build on Unix/Apple")

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.so")

if(NOT APPLE)
    # Linux or other unix
    set(rt_library rt)
    set(pthread_library pthread)
endif(NOT APPLE)

if(NOT "$ENV{OPENSSL_ROOT_DIR}" STREQUAL "")
    set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})
    set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
    message(STATUS "Setting up OpenSSL root and include vars to ${OPENSSL_ROOT_DIR}, ${OPENSSL_INCLUDE_DIR}")
endif()

find_package(OpenSSL REQUIRED)

set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIGINAL_LIB_SUFFIXES})

set(fc_sources
    src/uint128.cpp
    src/real128.cpp
    src/variant.cpp
    src/exception.cpp
    src/variant_object.cpp
    src/string.cpp
    src/time.cpp
    src/utf8.cpp
    src/io/datastream.cpp
    src/io/json.cpp
    src/io/varint.cpp
    src/io/fstream.cpp
    src/io/console.cpp
    src/filesystem.cpp
    src/interprocess/file_mapping.cpp
    src/interprocess/mmap_struct.cpp
    src/log/log_message.cpp
    src/log/logger.cpp
    src/log/appender.cpp
    src/log/console_appender.cpp
    src/log/gelf_appender.cpp
    src/log/logger_config.cpp
    src/crypto/_digest_common.cpp
    src/crypto/openssl.cpp
    src/crypto/aes.cpp
    src/crypto/crc.cpp
    src/crypto/city.cpp
    src/crypto/base36.cpp
    src/crypto/base58.cpp
    src/crypto/base64.cpp
    src/crypto/bigint.cpp
    src/crypto/hex.cpp
    src/crypto/sha1.cpp
    src/crypto/ripemd160.cpp
    src/crypto/sha256.cpp
    src/crypto/sha224.cpp
    src/crypto/sha512.cpp
    src/crypto/dh.cpp
    src/crypto/blowfish.cpp
    src/crypto/elliptic_common.cpp
    ${ECC_REST}
    src/crypto/elliptic_${ECC_IMPL}.cpp
    src/crypto/elliptic_r1.cpp
    src/crypto/rand.cpp
    src/crypto/public_key.cpp
    src/crypto/private_key.cpp
    src/crypto/signature.cpp
    src/network/ip.cpp
    src/network/platform_root_ca.cpp
    src/network/resolve.cpp
    src/network/udp_socket.cpp
    src/network/url.cpp
    src/network/http/http_client.cpp
    src/compress/smaz.cpp
    src/compress/zlib.cpp
    src/compress/miniz.c
)

set(fc_lite_sources
    src/variant.cpp
    src/exception.cpp
    src/variant_object.cpp
    src/string.cpp
    src/time.cpp
    src/utf8.cpp
    src/io/datastream.cpp
    src/io/json.cpp
    src/io/varint.cpp
    src/io/console.cpp
    src/filesystem.cpp
    src/log/log_message.cpp
    src/log/logger.cpp
    src/log/appender.cpp
    src/log/console_appender.cpp
    src/log/logger_config.cpp
    src/crypto/_digest_common.cpp
    src/crypto/openssl.cpp
    src/crypto/base58.cpp
    src/crypto/base64.cpp
    src/crypto/bigint.cpp
    src/crypto/hex.cpp
    src/crypto/ripemd160.cpp
    src/crypto/sha256.cpp
    src/crypto/sha512.cpp
    src/crypto/elliptic_common.cpp
    ${ECC_REST}
    src/crypto/elliptic_${ECC_IMPL}.cpp
    src/crypto/elliptic_r1.cpp
    src/crypto/rand.cpp
    src/crypto/public_key.cpp
    src/crypto/private_key.cpp
    src/crypto/signature.cpp
)

set(sources
    ${fc_sources}
)

set(lite_sources
    ${fc_lite_sources}
)

setup_library(fc SOURCES ${sources} LIBRARY_TYPE STATIC DONT_INSTALL_LIBRARY)
setup_library(fc_lite SOURCES ${lite_sources} LIBRARY_TYPE SHARED DONT_INSTALL_LIBRARY)
  
if(APPLE)
    # As of 10.10 yosemite, the OpenSSL static libraries shipped with os x have a dependency
    # on zlib, so any time you link in openssl you also need to link zlib.  .  We really want to detect whether openssl was configured with the --no-zlib
    # option or not when it was built, but that's difficult to do in practice, so we
    # just always try to link it in on mac.
    find_package(ZLIB REQUIRED)
else(APPLE)
    find_package(ZLIB)
endif(APPLE)

if(ZLIB_FOUND)
    message(STATUS "zlib found")
    add_definitions(-DHAS_ZLIB)
else()
    message(STATUS "zlib not found")
    set(ZLIB_LIBRARIES "")
endif(ZLIB_FOUND)

# This will become unnecessary once we update to websocketpp which fixes upstream issue #395
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWEBSOCKETPP_STRICT_MASKING")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_ASIO_HAS_STD_CHRONO")

target_include_directories(fc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${Secp256k1_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/websocketpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../rapidjson/include
)

target_include_directories(fc_lite PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${Secp256k1_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/websocketpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../rapidjson/include
)

if(NOT WIN32)
    set(LINK_USR_LOCAL_LIB -L/usr/local/lib)
endif()

if(APPLE)
    find_library(security_framework Security)
    find_library(corefoundation_framework CoreFoundation)
endif()

target_link_libraries(fc PUBLIC 
    fmt-header-only
    ${LINK_USR_LOCAL_LIB}
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_SPECifIC_LIBS}
    ${RPCRT4}
    ${CMAKE_DL_LIBS}
    ${rt_library}
    ${readline_libraries}
    ${ECC_LIB}
    ${security_framework}
    ${corefoundation_framework}
)

target_link_libraries(fc_lite PUBLIC
    fmt-header-only
    ${LINK_USR_LOCAL_LIB}
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${PLATFORM_SPECifIC_LIBS}
    ${RPCRT4}
    ${CMAKE_DL_LIBS}
    ${rt_library}
    ${ECC_SHARED_LIB}
)

set_target_properties(fc_lite PROPERTIES COMPILE_DEFINITIONS "FCLITE")

install(TARGETS fc_lite
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL)

set(OPENSSL_CONF_TARGET)
if(DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(OPENSSL_CONF_TARGET ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(OPENSSL_CONF_TARGET ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
