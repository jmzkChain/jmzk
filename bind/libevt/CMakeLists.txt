add_library( libevt evt_ecc.cpp )
set_target_properties( libevt PROPERTIES OUTPUT_NAME evt )

target_link_libraries( libevt
        PRIVATE evt_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   libevt

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
