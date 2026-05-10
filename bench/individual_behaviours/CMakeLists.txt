add_executable(Bench
    SessionEndBench.cpp
    SubmitCancelOrderBench.cpp
    SubmitLimitOrderAggressiveBench.cpp
    SubmitLimitOrderFOKBench.cpp
    SubmitLimitOrderIOCBench.cpp
    SubmitLimitOrderRestingBench.cpp
    SubmitMarketOrderBench.cpp
    SubmitModifyOrderBench.cpp
)

target_include_directories(Bench
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(Bench
    PRIVATE
        OrderBook
        benchmark::benchmark_main
)

target_compile_features(Bench PRIVATE cxx_std_20)