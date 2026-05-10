add_executable(Bench
    MixedWorkloadBench.cpp
)

target_include_directories(Bench
    PRIVATE
        ${PROJECT_SOURCE_DIR}/bench/mixed_workload/data
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(Bench
    PRIVATE
        OrderBook
)

target_compile_features(Bench PRIVATE cxx_std_20)