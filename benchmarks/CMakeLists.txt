find_package( benchmark REQUIRED )

add_executable( evt_benchmarks 
    main.cpp
    json.cpp
    actions.cpp
    )
target_link_libraries( evt_benchmarks evt_chain evt_testing fc ${BENCHMARK_LIBRARIES} )
