add_library(libjmzk SHARED 
    jmzk.cpp
    jmzk_ecc.cpp
    jmzk_abi.cpp
    jmzk_address.cpp
    jmzk_jmzklink.cpp
)
set_target_properties(libjmzk PROPERTIES OUTPUT_NAME jmzk)

target_link_libraries(libjmzk PUBLIC
    jmzk_chain_lite fc_lite ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

target_include_directories(libjmzk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

install(
    TARGETS libjmzk
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)

if(ENABLE_TESTING)
    add_subdirectory(test)
endif()
