add_library(OrderBook Orderbook.cpp)

add_subdirectory(core)

target_include_directories(OrderBook PUBLIC include)

target_link_libraries(OrderBook PRIVATE core)
