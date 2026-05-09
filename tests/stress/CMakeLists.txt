add_executable(StressTests 
    OBRandomStressTests.cpp
)

target_include_directories(StressTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/core
)

target_link_libraries(StressTests
    PRIVATE
        OrderBook
        GTest::gtest_main
)

enable_sanitizers(UnitTests)

include(GoogleTest)
gtest_discover_tests(StressTests)