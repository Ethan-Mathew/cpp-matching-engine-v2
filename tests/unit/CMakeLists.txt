add_executable(UnitTests PriceLevelTests.cpp)

target_link_libraries(UnitTests
    PRIVATE
        OrderBook
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(UnitTests)