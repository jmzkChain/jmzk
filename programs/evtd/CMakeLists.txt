add_executable(jmzkd main.cpp)
if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

find_package(Jemalloc QUIET)
if(JEMALLOC_FOUND)
    message(STATUS "Found jemalloc; compiling jmzkd with jemalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS jemalloc)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../.git)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        OUTPUT_VARIABLE "jmzkd_BUILD_VERSION"
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        message(STATUS "Git commit revision: ${jmzkd_BUILD_VERSION}")
    else()
        set(jmzkd_BUILD_VERSION 0)
    endif()
endif()

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(jmzkd PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
    if(APPLE)
        set(whole_archive_flag "-force_load")
        set(no_whole_archive_flag "")
        set_target_properties(jmzkd PROPERTIES LINK_FLAGS "${LUAJIT_LINK_FLAGS}")
    else()
        set(whole_archive_flag "--whole-archive")
        set(no_whole_archive_flag "--no-whole-archive")
    endif()
else()
    set(whole_archive_flag "--whole-archive")
    set(no_whole_archive_flag "--no-whole-archive")
endif()

target_link_libraries(jmzkd
    PRIVATE appbase
    PRIVATE chain_plugin
    PRIVATE producer_plugin
    PRIVATE net_plugin
    PRIVATE http_plugin
    PRIVATE http_client_plugin
    PRIVATE jmzk_chain
    PRIVATE fc
    PRIVATE -Wl,${whole_archive_flag} staking_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} producer_api_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} chain_api_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} net_api_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} jmzk_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} jmzk_api_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} jmzk_link_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} bnet_plugin -Wl,${no_whole_archive_flag}
    PRIVATE -Wl,${whole_archive_flag} trafficgen_plugin -Wl,${no_whole_archive_flag}
    PRIVATE ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

if(ENABLE_POSTGRES_SUPPORT)
    target_link_libraries(jmzkd
        PRIVATE -Wl,${whole_archive_flag} postgres_plugin -Wl,${no_whole_archive_flag}
        PRIVATE -Wl,${whole_archive_flag} history_plugin -Wl,${no_whole_archive_flag}
        PRIVATE -Wl,${whole_archive_flag} history_api_plugin -Wl,${no_whole_archive_flag}
   )
endif()

if(ENABLE_BREAKPAD_SUPPORT)
    find_package(breakpad REQUIRED)

    target_compile_definitions(jmzkd PUBLIC BREAKPAD_SUPPORT)
    target_link_libraries(jmzkd PRIVATE ${BREAKPAD_LIBRARIES})
    target_include_directories(jmzkd PUBLIC ${BREAKPAD_INCLUDE_DIR})
endif()

install(TARGETS jmzkd
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
)
