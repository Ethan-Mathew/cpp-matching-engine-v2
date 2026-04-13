#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/SubmissionResults.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>
#include <cstdint>

using namespace lob;

constexpr std::size_t initialSlabSize = 10;
constexpr Price defaultPrice = 10000;

class OBSubmitLimitOrderTest : public testing::Test
{
protected:
    OBSubmitLimitOrderTest()
        : ob_{initialSlabSize}
    {
    }

    static LimitOrderRequest make_order(OrderID id,
                                        Price price,
                                        Quantity qty,
                                        Side side,
                                        TimeInForce tif = TimeInForce::GTC)
    {
        return LimitOrderRequest{id, price, qty, side, tif};
    }

    void expect_empty_book()
    {
        EXPECT_EQ(ob_.get_num_orders(), 0);
        EXPECT_EQ(ob_.get_num_levels_bids(), 0);
        EXPECT_EQ(ob_.get_num_levels_asks(), 0);
        EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
    }

    OrderBook ob_;
};

TEST_F(OBSubmitLimitOrderTest, OrderBookConstructs)
{
    EXPECT_EQ(ob_.get_memory_pool_size(), initialSlabSize);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderTest, SubmitSellSideOrderRests)
{
    SubmissionResult result = ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    EXPECT_EQ(result.quantityRequested_, 1);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 1);
}

TEST_F(OBSubmitLimitOrderTest, SubmitBuySideOrderRests)
{
    SubmissionResult result = ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    EXPECT_EQ(result.quantityRequested_, 1);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 1);
}

TEST_F(OBSubmitLimitOrderTest, MultipleSellOrdersAtSameLevelRest)
{
    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        SubmissionResult result = ob_.submit_limit_order(
            make_order(static_cast<OrderID>(i), defaultPrice, static_cast<Quantity>(i), Side::SELL));
        EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    }

    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_orders(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 55);
}

TEST_F(OBSubmitLimitOrderTest, MultipleBuyOrdersAtSameLevelRest)
{
    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        SubmissionResult result = ob_.submit_limit_order(
            make_order(static_cast<OrderID>(i), defaultPrice, static_cast<Quantity>(i), Side::BUY));
        EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    }

    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_orders(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 55);
}

TEST_F(OBSubmitLimitOrderTest, MultipleSellOrdersAtDifferentLevelsRest)
{
    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        SubmissionResult result = ob_.submit_limit_order(
            make_order(static_cast<OrderID>(i),
                       static_cast<Price>(defaultPrice + i),
                       static_cast<Quantity>(i),
                       Side::SELL));
        EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    }

    EXPECT_EQ(ob_.get_num_levels_asks(), initialSlabSize);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_orders(), initialSlabSize);
}

TEST_F(OBSubmitLimitOrderTest, MultipleBuyOrdersAtDifferentLevelsRest)
{
    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        SubmissionResult result = ob_.submit_limit_order(
            make_order(static_cast<OrderID>(i),
                       static_cast<Price>(defaultPrice + i),
                       static_cast<Quantity>(i),
                       Side::BUY));
        EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    }

    EXPECT_EQ(ob_.get_num_levels_bids(), initialSlabSize);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), initialSlabSize);
    EXPECT_EQ(ob_.get_num_orders(), initialSlabSize);
}

TEST_F(OBSubmitLimitOrderTest, AggressiveSellFullyMatchesAndDoesNotRest)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::BUY));

    SubmissionResult result = ob_.submit_limit_order(make_order(2, defaultPrice, 1, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 1);
    EXPECT_EQ(result.quantityFilled_, 1);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 1);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderTest, AggressiveBuyFullyMatchesAndDoesNotRest)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::SELL));

    SubmissionResult result = ob_.submit_limit_order(make_order(2, defaultPrice, 1, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 1);
    EXPECT_EQ(result.quantityFilled_, 1);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 1);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderTest, AggressiveSellPartiallyFillsThenRestsRemainder)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::BUY));

    SubmissionResult result = ob_.submit_limit_order(make_order(2, defaultPrice, 2, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_RESTING);
    EXPECT_EQ(result.quantityRequested_, 2);
    EXPECT_EQ(result.quantityFilled_, 1);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 1);

    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}

TEST_F(OBSubmitLimitOrderTest, AggressiveBuyPartiallyFillsThenRestsRemainder)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 1, Side::SELL));

    SubmissionResult result = ob_.submit_limit_order(make_order(2, defaultPrice, 2, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_RESTING);
    EXPECT_EQ(result.quantityRequested_, 2);
    EXPECT_EQ(result.quantityFilled_, 1);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 1);

    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}

TEST_F(OBSubmitLimitOrderTest, DuplicateOrderIdIsRejectedAndDoesNotMutateBook)
{
    SubmissionResult first = ob_.submit_limit_order(make_order(1, defaultPrice, 5, Side::BUY));
    SubmissionResult second = ob_.submit_limit_order(make_order(1, defaultPrice + 10, 7, Side::SELL));

    EXPECT_EQ(first.status_, SubmitStatus::RESTING);
    EXPECT_EQ(second.status_, SubmitStatus::REJECTED);
    EXPECT_EQ(second.quantityRequested_, 7);
    EXPECT_EQ(second.quantityFilled_, 0);
    EXPECT_TRUE(second.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 5);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}

TEST_F(OBSubmitLimitOrderTest, NonCrossingOrderRestsInsteadOfExecuting)
{
    ob_.submit_limit_order(make_order(1, 10010, 3, Side::SELL));

    SubmissionResult result = ob_.submit_limit_order(make_order(2, 10000, 3, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::RESTING);
    EXPECT_EQ(result.quantityRequested_, 3);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
}

TEST_F(OBSubmitLimitOrderTest, MultiLevelSweepProducesMultipleExecutions)
{
    ob_.submit_limit_order(make_order(1, 10000, 2, Side::SELL));
    ob_.submit_limit_order(make_order(2, 10001, 3, Side::SELL));

    SubmissionResult result = ob_.submit_limit_order(make_order(3, 10001, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderTest, SamePriceFIFOIsPreserved)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::SELL));
    ob_.submit_limit_order(make_order(2, defaultPrice, 3, Side::SELL));

    SubmissionResult result = ob_.submit_limit_order(make_order(3, defaultPrice, 4, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 4);
    EXPECT_EQ(result.quantityFilled_, 4);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 2);

    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 1);
}