find_package( benchmark REQUIRED )

add_executable( benchmarks 
    main.cpp
    json.cpp)
target_link_libraries( benchmarks evt_chain fc ${BENCHMARK_LIBRARIES} )
