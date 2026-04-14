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

class OBSubmitMarketOrderTest : public testing::Test
{
protected:
    OBSubmitMarketOrderTest()
        : ob_{initialSlabSize}
    {
    }

    static LimitOrderRequest make_limit_order(OrderID id,
                                              Price price,
                                              Quantity qty,
                                              Side side,
                                              TimeInForce tif = TimeInForce::GTC)
    {
        return LimitOrderRequest{id, price, qty, side, tif};
    }

    static MarketOrderRequest make_market_order(OrderID id,
                                                Quantity qty,
                                                Side side)
    {
        return MarketOrderRequest{id, qty, side};
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

TEST_F(OBSubmitMarketOrderTest, MarketOrderOnEmptyBookCancels)
{
    SubmissionResult result = ob_.submit_market_order(
        make_market_order(1, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::CANCELED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);
    EXPECT_TRUE(result.executions_.empty());

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, DuplicateMarketOrderIdIsRejected)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(1, 3, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::REJECTED);
    EXPECT_EQ(result.quantityRequested_, 3);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}

TEST_F(OBSubmitMarketOrderTest, BuyMarketOrderFullyFillsSingleAsk)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 5);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, SellMarketOrderFullyFillsSingleBid)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 6, Side::BUY));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 6, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 6);
    EXPECT_EQ(result.quantityFilled_, 6);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 6);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, BuyMarketOrderPartiallyFillsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 3, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 3);
    EXPECT_EQ(result.get_quantity_remaining(), 2);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, SellMarketOrderPartiallyFillsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 4, Side::BUY));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 7, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 7);
    EXPECT_EQ(result.quantityFilled_, 4);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    ASSERT_EQ(result.executions_.size(), 1);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 4);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, BuyMarketOrderSweepsMultipleAskLevelsAndFills)
{
    ob_.submit_limit_order(make_limit_order(1, 10000, 2, Side::SELL));
    ob_.submit_limit_order(make_limit_order(2, 10001, 3, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(3, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, SellMarketOrderSweepsMultipleBidLevelsAndFills)
{
    ob_.submit_limit_order(make_limit_order(1, 10001, 2, Side::BUY));
    ob_.submit_limit_order(make_limit_order(2, 10000, 3, Side::BUY));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(3, 5, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, BuyMarketOrderSweepsMultipleAskLevelsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_limit_order(1, 10000, 2, Side::SELL));
    ob_.submit_limit_order(make_limit_order(2, 10001, 3, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(3, 8, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 8);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 3);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, SellMarketOrderSweepsMultipleBidLevelsAndCancelsRemainder)
{
    ob_.submit_limit_order(make_limit_order(1, 10001, 2, Side::BUY));
    ob_.submit_limit_order(make_limit_order(2, 10000, 3, Side::BUY));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(3, 9, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityRequested_, 9);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 4);
    ASSERT_EQ(result.executions_.size(), 2);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitMarketOrderTest, MarketOrderNeverRestsResidualOnBuySide)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 2, Side::SELL));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 6, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityFilled_, 2);
    EXPECT_EQ(result.get_quantity_remaining(), 4);

    EXPECT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
}

TEST_F(OBSubmitMarketOrderTest, MarketOrderNeverRestsResidualOnSellSide)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 2, Side::BUY));

    SubmissionResult result = ob_.submit_market_order(
        make_market_order(2, 7, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::PARTIALLY_FILLED_CANCELED);
    EXPECT_EQ(result.quantityFilled_, 2);
    EXPECT_EQ(result.get_quantity_remaining(), 5);

    EXPECT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
}