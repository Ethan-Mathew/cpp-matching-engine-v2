add_executable(Latency LatencyMeasurementMain.cpp)

target_include_directories(Latency
    PRIVATE
        ${PROJECT_SOURCE_DIR}/bench/latency
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(Latency
    PRIVATE
        OrderBook
)