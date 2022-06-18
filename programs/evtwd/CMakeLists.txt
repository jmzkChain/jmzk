add_executable(jmzkwd main.cpp)

if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

find_package(Jemalloc QUIET)
if(JEMALLOC_FOUND)
    message(STATUS "Found jemalloc; compiling jmzkwd with jemalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS jemalloc)
endif()

target_link_libraries(jmzkwd
    PRIVATE appbase
    PRIVATE wallet_api_plugin wallet_plugin
    PRIVATE http_plugin
    PRIVATE jmzk_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

install(
    TARGETS jmzkwd
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
)
