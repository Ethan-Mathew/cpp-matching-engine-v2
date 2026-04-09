#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderRequests.hpp"
#include "lob/OrderType.hpp"
#include "lob/Side.hpp"
#include "lob/SubmissionResults.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

constexpr std::size_t initialSlabSize = 10;

class OBSubmitLimitOrderTest : public testing::Test
{
protected:
    OBSubmitLimitOrderTest()
        : ob_{initialSlabSize}
    {
    }

    OrderBook ob_;
};

TEST_F(OBSubmitLimitOrderTest, OrderBookConstructs)
{
    ASSERT_EQ(ob_.get_memory_pool_size(), 10);
    ASSERT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
}