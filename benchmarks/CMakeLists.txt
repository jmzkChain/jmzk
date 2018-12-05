find_package( benchmark REQUIRED )

add_executable( evt_benchmarks 
    main.cpp
    json.cpp
    actions.cpp
    ecc.cpp
    sha256.cpp
    sha256/intrinsics.cpp
    # sha256/cryptopp.cpp
    sha256/fc.cpp
    sha256/cgminer.cpp
    )
target_link_libraries( evt_benchmarks evt_chain evt_testing fc ${BENCHMARK_LIBRARIES} )
# target_link_libraries( cryptopp )
