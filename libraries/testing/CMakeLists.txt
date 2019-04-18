## SORT .cpp by most likely to change / break compile
add_library(evt_testing
    tester.cpp
    tester_network.cpp
)

target_link_libraries(evt_testing evt_chain fc chainbase)
target_include_directories(evt_testing PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)

install(
    TARGETS evt_testing
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/evt/testing)
