#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Results.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>
#include <cstdint>

using namespace lob;

constexpr std::size_t initialSlabSize = 10;
constexpr Price defaultPrice = 10000;

class OBSubmitLimitOrderIOCTest : public testing::Test
{
protected:
    OBSubmitLimitOrderIOCTest()
        : ob_{initialSlabSize}
    {
    }

    static LimitOrderRequest make_order(OrderID id,
                                        Price price,
                                        Quantity qty,
                                        Side side,
                                        TimeInForce tif = TimeInForce::IOC)
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

TEST_F(OBSubmitLimitOrderIOCTest, OnEmptyBookCancels)
{
    SubmissionResult result = ob_.submit_limit_order(
        make_order(1, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::CANCELED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);
    EXPECT_TRUE(result.executions_.empty());

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, NonCrossingBuyCancelsWithoutResting)
{
    ob_.submit_limit_order(make_order(1, defaultPrice + 10, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 3, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::CANCELED);
    EXPECT_EQ(result.quantityRequested_, 3);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice + 10, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice + 10, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice + 10, Side::SELL), 3);
}

TEST_F(OBSubmitLimitOrderIOCTest, NonCrossingSellCancelsWithoutResting)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 4, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice + 10, 4, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::CANCELED);
    EXPECT_EQ(result.quantityRequested_, 4);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 4);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 4);
}

TEST_F(OBSubmitLimitOrderIOCTest, BuyFullyFillsSingleAsk)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 5, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 5);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, SellFullyFillsSingleBid)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 6, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 6, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 6);
    EXPECT_EQ(result.quantityFilled_, 6);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 6);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, BuyPartiallyFillsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 3);
    EXPECT_EQ(result.get_quantity_remaining(), 2);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, SellPartiallyFillsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 4, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 7, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 7);
    EXPECT_EQ(result.quantityFilled_, 4);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 4);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, BuySweepsMultipleLevelsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_order(1, 10000, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10001, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10001, 7, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 7);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 2);
    ASSERT_EQ(result.executions_.size(), 2u);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, SellSweepsMultipleLevelsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_order(1, 10001, 2, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10000, 3, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10000, 8, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 8);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    ASSERT_EQ(result.executions_.size(), 2u);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderIOCTest, NeverRestsResidualOnBuySide)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityFilled_, 2);
    EXPECT_EQ(result.get_quantity_remaining(), 3);

    EXPECT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
}

TEST_F(OBSubmitLimitOrderIOCTest, NeverRestsResidualOnSellSide)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 6, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityFilled_, 2);
    EXPECT_EQ(result.get_quantity_remaining(), 4);

    EXPECT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
}

TEST_F(OBSubmitLimitOrderIOCTest, DuplicateOrderIdIsRejected)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 5, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(1, defaultPrice, 5, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::REJECTED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}