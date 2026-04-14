#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
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

class OBCancelOrderTest : public testing::Test
{
protected:
    OBCancelOrderTest()
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

    static CancelOrderRequest make_cancel(OrderID id)
    {
        return CancelOrderRequest{id};
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

TEST_F(OBCancelOrderTest, CancelMissingOrderReturnsNotFound)
{
    CancelResult result = ob_.cancel_order(make_cancel(42));

    EXPECT_EQ(result.status_, CancelStatus::NOT_FOUND);
    EXPECT_EQ(result.quantityCancelled_, 0);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelSingleBidOrderRemovesIt)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY));

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 5);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelSingleAskOrderRemovesIt)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 7, Side::SELL));

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 7);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelOneOfMultipleOrdersAtSameBidLevelKeepsLevel)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 2, Side::BUY));
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 3, Side::BUY));

    ASSERT_EQ(ob_.get_num_orders(), 2);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 2);
    ASSERT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 5);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 2);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 3);
}

TEST_F(OBCancelOrderTest, CancelOneOfMultipleOrdersAtSameAskLevelKeepsLevel)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 4, Side::SELL));
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 6, Side::SELL));

    ASSERT_EQ(ob_.get_num_orders(), 2);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);
    ASSERT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 2);
    ASSERT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 10);

    CancelResult result = ob_.cancel_order(make_cancel(2));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 6);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 4);
}

TEST_F(OBCancelOrderTest, CancelOnlyOrderAtBidLevelErasesLevel)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 8, Side::BUY));

    ASSERT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 8);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelOnlyOrderAtAskLevelErasesLevel)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 9, Side::SELL));

    ASSERT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 9);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelOrderAtOneBidLevelLeavesOtherBidLevelsUntouched)
{
    ob_.submit_limit_order(make_limit_order(1, 10000, 2, Side::BUY));
    ob_.submit_limit_order(make_limit_order(2, 10001, 3, Side::BUY));
    ob_.submit_limit_order(make_limit_order(3, 10002, 4, Side::BUY));

    ASSERT_EQ(ob_.get_num_orders(), 3);
    ASSERT_EQ(ob_.get_num_levels_bids(), 3);

    CancelResult result = ob_.cancel_order(make_cancel(2));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 3);

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 2);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);

    EXPECT_TRUE(ob_.check_level_exists(10000, Side::BUY));
    EXPECT_FALSE(ob_.check_level_exists(10001, Side::BUY));
    EXPECT_TRUE(ob_.check_level_exists(10002, Side::BUY));

    EXPECT_EQ(ob_.get_num_shares_at_level(10000, Side::BUY), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10002, Side::BUY), 4);
}

TEST_F(OBCancelOrderTest, CancelOrderAtOneAskLevelLeavesOtherAskLevelsUntouched)
{
    ob_.submit_limit_order(make_limit_order(1, 10000, 2, Side::SELL));
    ob_.submit_limit_order(make_limit_order(2, 10001, 3, Side::SELL));
    ob_.submit_limit_order(make_limit_order(3, 10002, 4, Side::SELL));

    ASSERT_EQ(ob_.get_num_orders(), 3);
    ASSERT_EQ(ob_.get_num_levels_asks(), 3);

    CancelResult result = ob_.cancel_order(make_cancel(2));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 3);

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 2);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);

    EXPECT_TRUE(ob_.check_level_exists(10000, Side::SELL));
    EXPECT_FALSE(ob_.check_level_exists(10001, Side::SELL));
    EXPECT_TRUE(ob_.check_level_exists(10002, Side::SELL));

    EXPECT_EQ(ob_.get_num_shares_at_level(10000, Side::SELL), 2);
    EXPECT_EQ(ob_.get_num_shares_at_level(10002, Side::SELL), 4);
}

TEST_F(OBCancelOrderTest, CancelAfterPartialExecutionCancelsRemainingOpenQuantity)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 10, Side::SELL));

    SubmissionResult fillResult =
        ob_.submit_limit_order(make_limit_order(2, defaultPrice, 4, Side::BUY, TimeInForce::IOC));

    ASSERT_EQ(fillResult.status_, SubmitStatus::FILLED);
    ASSERT_EQ(fillResult.quantityFilled_, 4);

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);
    ASSERT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 6);

    CancelResult cancelResult = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(cancelResult.status_, CancelStatus::CANCELED);
    EXPECT_EQ(cancelResult.quantityCancelled_, 6);

    expect_empty_book();
}

TEST_F(OBCancelOrderTest, CancelDoesNotTouchOppositeSideBook)
{
    ob_.submit_limit_order(make_limit_order(1,  9999, 2, Side::BUY));
    ob_.submit_limit_order(make_limit_order(2, 10001, 3, Side::SELL));

    ASSERT_EQ(ob_.get_num_orders(), 2);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);

    CancelResult result = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(result.status_, CancelStatus::CANCELED);
    EXPECT_EQ(result.quantityCancelled_, 2);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    EXPECT_TRUE(ob_.check_level_exists(10001, Side::SELL));
    EXPECT_EQ(ob_.get_num_shares_at_level(10001, Side::SELL), 3);
}

TEST_F(OBCancelOrderTest, ReCancelingSameOrderReturnsNotFound)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY));

    CancelResult first = ob_.cancel_order(make_cancel(1));
    CancelResult second = ob_.cancel_order(make_cancel(1));

    EXPECT_EQ(first.status_, CancelStatus::CANCELED);
    EXPECT_EQ(first.quantityCancelled_, 5);

    EXPECT_EQ(second.status_, CancelStatus::NOT_FOUND);
    EXPECT_EQ(second.quantityCancelled_, 0);

    expect_empty_book();
}