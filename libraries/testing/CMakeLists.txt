## SORT .cpp by most likely to change / break compile
add_library(jmzk_testing
    tester.cpp
    tester_network.cpp
)

target_link_libraries(jmzk_testing jmzk_chain fc chainbase)
target_include_directories(jmzk_testing PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)

install(
    TARGETS jmzk_testing
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} OPTIONAL
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} OPTIONAL
)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_INCLUDEDIR}/jmzk/testing)
