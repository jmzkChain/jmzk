find_package( benchmark REQUIRED )

add_executable( jmzk_benchmarks 
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
target_link_libraries( jmzk_benchmarks jmzk_chain jmzk_testing fc ${BENCHMARK_LIBRARIES} )
# target_link_libraries( cryptopp )
