file(GLOB UNIT_TESTS "*.cpp")
add_executable( libevt_test ${UNIT_TESTS}  )
target_link_libraries( libevt_test  libevt ${Boost_LIBRARIES} )
