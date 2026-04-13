add_executable(UnitTests 
    MemoryPoolTests.cpp
    OBSessionEndTests.cpp
    OBSubmitLimitOrderFOKTests.cpp
    OBSubmitLimitOrderIOCTests.cpp
    OBSubmitLimitOrderRestingTests.cpp
    OBSubmitMarketOrderTests.cpp
    PriceLevelTests.cpp
)

target_include_directories(UnitTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/core
)

target_link_libraries(UnitTests
    PRIVATE
        OrderBook
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(UnitTests)