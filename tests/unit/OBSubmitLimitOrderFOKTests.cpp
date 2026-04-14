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

class OBSubmitLimitOrderFOKTest : public testing::Test
{
protected:
    OBSubmitLimitOrderFOKTest()
        : ob_{initialSlabSize}
    {
    }

    static LimitOrderRequest make_order(OrderID id,
                                        Price price,
                                        Quantity qty,
                                        Side side,
                                        TimeInForce tif = TimeInForce::FOK)
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

TEST_F(OBSubmitLimitOrderFOKTest, OnEmptyBookIsKilled)
{
    SubmissionResult result = ob_.submit_limit_order(
        make_order(1, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);
    EXPECT_TRUE(result.executions_.empty());

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderFOKTest, NonCrossingBuyIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, defaultPrice + 10, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 3, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
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

TEST_F(OBSubmitLimitOrderFOKTest, NonCrossingSellIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 4, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice + 10, 4, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
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

TEST_F(OBSubmitLimitOrderFOKTest, BuyInsufficientSingleLevelIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 3);
}

TEST_F(OBSubmitLimitOrderFOKTest, SellInsufficientSingleLevelIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 6, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 6);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 6);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 2);
}

TEST_F(OBSubmitLimitOrderFOKTest, BuyInsufficientAcrossMultipleLevelsIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, 10000, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10001, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10001, 6, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 6);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 6);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_asks(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10000, Side::SELL), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10001, Side::SELL), 3);
}

TEST_F(OBSubmitLimitOrderFOKTest, SellInsufficientAcrossMultipleLevelsIsKilledWithoutMutation)
{
    ob_.submit_limit_order(make_order(1, 10001, 2, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10000, 3, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10000, 7, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 7);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 7);
    EXPECT_TRUE(result.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 2);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10001, Side::BUY), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10000, Side::BUY), 3);
}

TEST_F(OBSubmitLimitOrderFOKTest, BuyExactLiquidityFullyFills)
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

TEST_F(OBSubmitLimitOrderFOKTest, SellExactLiquidityFullyFills)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 4, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 4, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 4);
    EXPECT_EQ(result.quantityFilled_, 4);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 1u);
    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 4);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderFOKTest, BuySufficientAcrossMultipleLevelsFullyFills)
{
    ob_.submit_limit_order(make_order(1, 10000, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10001, 3, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10001, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 2u);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderFOKTest, SellSufficientAcrossMultipleLevelsFullyFills)
{
    ob_.submit_limit_order(make_order(1, 10001, 2, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10000, 3, Side::BUY, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(3, 10000, 5, Side::SELL));

    EXPECT_EQ(result.status_, SubmitStatus::FILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 5);
    EXPECT_EQ(result.get_quantity_remaining(), 0);
    ASSERT_EQ(result.executions_.size(), 2u);

    EXPECT_EQ(result.executions_[0].makerOrderID_, 1);
    EXPECT_EQ(result.executions_[0].makerPrice_, 10001);
    EXPECT_EQ(result.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(result.executions_[1].makerOrderID_, 2);
    EXPECT_EQ(result.executions_[1].makerPrice_, 10000);
    EXPECT_EQ(result.executions_[1].executedQuantity_, 3);

    expect_empty_book();
}

TEST_F(OBSubmitLimitOrderFOKTest, FailedFOKNeverRestsResidual)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::SELL, TimeInForce::GTC));

    SubmissionResult result = ob_.submit_limit_order(
        make_order(2, defaultPrice, 5, Side::BUY));

    EXPECT_EQ(result.status_, SubmitStatus::KILLED);
    EXPECT_EQ(result.quantityRequested_, 5);
    EXPECT_EQ(result.quantityFilled_, 0);
    EXPECT_EQ(result.get_quantity_remaining(), 5);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
}

TEST_F(OBSubmitLimitOrderFOKTest, DuplicateOrderIdIsRejectedBeforeMatching)
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