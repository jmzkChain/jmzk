add_executable(evtc main.cpp httpc.cpp help_text.cpp localize.hpp config.hpp CLI11.hpp)

if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

find_package(Jemalloc QUIET)
if(JEMALLOC_FOUND)
    message( STATUS "Found jemalloc; compiling evtc with jemalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS jemalloc)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        OUTPUT_VARIABLE "evtc_BUILD_VERSION"
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "Git commit revision: ${evtc_BUILD_VERSION}")
    else()
        set(evtc_BUILD_VERSION 0)
    endif()
endif()

find_package(Intl REQUIRED)

set(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
set(LOCALEDOMAIN evtc)
configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(evtc PUBLIC ${Intl_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(evtc PRIVATE
    chain_plugin http_plugin evt_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} ${Intl_LIBRARIES})

install(TARGETS evtc
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL 
)
