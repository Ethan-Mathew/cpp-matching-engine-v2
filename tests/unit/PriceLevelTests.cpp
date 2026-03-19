#include <gtest/gtest.h>

#include "lob/Aliases.hpp"

#include "PriceLevel.hpp"
#include "RestingOrder.hpp"
#include "RestingLifetime.hpp"

using namespace lob::core;



TEST(PriceLevelTest, PriceLevelConstructsEmpty)
{
    PriceLevel level{100};

    ASSERT_TRUE(level.empty());
}

/*
TEST(PriceLevelTest, AcceptsNewOrders)
{
    RestingOrder newOrder{0, 100, RestingLifetime::GTC};
    PriceLevel level{100};

    level
}
*/

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}