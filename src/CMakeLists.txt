add_library(OrderBook
    Orderbook.cpp
    core/PriceLevel.cpp
    core/MemoryPool.cpp
)

target_include_directories(OrderBook
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(OrderBook PUBLIC cxx_std_20)