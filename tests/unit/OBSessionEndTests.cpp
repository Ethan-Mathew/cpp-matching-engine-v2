#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/DayOrderPruneResult.hpp"
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

class OBSessionEndTest : public testing::Test
{
protected:
    OBSessionEndTest()
        : ob_{initialSlabSize}
    {
    }

    static LimitOrderRequest make_order(OrderID id,
                                        Price price,
                                        Quantity qty,
                                        Side side,
                                        TimeInForce tif)
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

TEST_F(OBSessionEndTest, SessionEndOnEmptyBookDoesNothing)
{
    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 0);
    EXPECT_EQ(result.sharesErased, 0);
    EXPECT_EQ(result.priceLevelsErased, 0);

    expect_empty_book();
}

TEST_F(OBSessionEndTest, SessionEndPrunesSingleDayBid)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 5, Side::BUY, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 1);
    EXPECT_EQ(result.sharesErased, 5);
    EXPECT_EQ(result.priceLevelsErased, 1);

    expect_empty_book();
}

TEST_F(OBSessionEndTest, SessionEndPrunesSingleDayAsk)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 7, Side::SELL, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 1);
    EXPECT_EQ(result.sharesErased, 7);
    EXPECT_EQ(result.priceLevelsErased, 1);

    expect_empty_book();
}

TEST_F(OBSessionEndTest, SessionEndLeavesSingleGtcBidUntouched)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 5, Side::BUY, TimeInForce::GTC));

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 0);
    EXPECT_EQ(result.sharesErased, 0);
    EXPECT_EQ(result.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 5);
}

TEST_F(OBSessionEndTest, SessionEndLeavesSingleGtcAskUntouched)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 6, Side::SELL, TimeInForce::GTC));

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 0);
    EXPECT_EQ(result.sharesErased, 0);
    EXPECT_EQ(result.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 6);
}

TEST_F(OBSessionEndTest, SessionEndPrunesOnlyDayOrdersAtSameBidLevel)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::BUY, TimeInForce::DAY));
    ob_.submit_limit_order(make_order(2, defaultPrice, 3, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(3, defaultPrice, 4, Side::BUY, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 3);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 3);
    ASSERT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 9);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 2);
    EXPECT_EQ(result.sharesErased, 6);
    EXPECT_EQ(result.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 3);
}

TEST_F(OBSessionEndTest, SessionEndPrunesOnlyDayOrdersAtSameAskLevel)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::SELL, TimeInForce::DAY));
    ob_.submit_limit_order(make_order(2, defaultPrice, 3, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(3, defaultPrice, 4, Side::SELL, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 3);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);
    ASSERT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 3);
    ASSERT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 9);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 2);
    EXPECT_EQ(result.sharesErased, 6);
    EXPECT_EQ(result.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 3);
}

TEST_F(OBSessionEndTest, SessionEndErasesLevelWhenAllOrdersAtBidLevelAreDay)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::BUY, TimeInForce::DAY));
    ob_.submit_limit_order(make_order(2, defaultPrice, 3, Side::BUY, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 2);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 2);
    EXPECT_EQ(result.sharesErased, 5);
    EXPECT_EQ(result.priceLevelsErased, 1);

    expect_empty_book();
}

TEST_F(OBSessionEndTest, SessionEndErasesLevelWhenAllOrdersAtAskLevelAreDay)
{
    ob_.submit_limit_order(make_order(1, defaultPrice, 2, Side::SELL, TimeInForce::DAY));
    ob_.submit_limit_order(make_order(2, defaultPrice, 3, Side::SELL, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 2);
    ASSERT_EQ(ob_.get_num_levels_asks(), 1);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 2);
    EXPECT_EQ(result.sharesErased, 5);
    EXPECT_EQ(result.priceLevelsErased, 1);

    expect_empty_book();
}

TEST_F(OBSessionEndTest, SessionEndPrunesBothSidesAndReportsTotals)
{
    ob_.submit_limit_order(make_order(1,  9999, 2, Side::BUY,  TimeInForce::DAY));
    ob_.submit_limit_order(make_order(2, 10000, 3, Side::BUY,  TimeInForce::GTC));
    ob_.submit_limit_order(make_order(3, 10001, 4, Side::SELL, TimeInForce::DAY));
    ob_.submit_limit_order(make_order(4, 10002, 5, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_order(5, 10003, 6, Side::SELL, TimeInForce::DAY));

    ASSERT_EQ(ob_.get_num_orders(), 5);
    ASSERT_EQ(ob_.get_num_levels_bids(), 2);
    ASSERT_EQ(ob_.get_num_levels_asks(), 3);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 5);

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 3);
    EXPECT_EQ(result.sharesErased, 12);
    EXPECT_EQ(result.priceLevelsErased, 3);

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);

    EXPECT_TRUE(ob_.check_level_exists(10000, Side::BUY));
    EXPECT_TRUE(ob_.check_level_exists(10002, Side::SELL));

    EXPECT_EQ(ob_.get_num_orders_at_level(10000, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_orders_at_level(10002, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(10000, Side::BUY), 3);
    EXPECT_EQ(ob_.get_num_shares_at_level(10002, Side::SELL), 5);
}

TEST_F(OBSessionEndTest, SessionEndDoesNotRemoveAnythingWhenOnlyGtcOrdersExist)
{
    ob_.submit_limit_order(make_order(1,  9999, 2, Side::BUY,  TimeInForce::GTC));
    ob_.submit_limit_order(make_order(2, 10001, 4, Side::SELL, TimeInForce::GTC));

    DayOrderPruneResult result = ob_.on_session_end();

    EXPECT_EQ(result.ordersPruned, 0);
    EXPECT_EQ(result.sharesErased, 0);
    EXPECT_EQ(result.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 2);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);
}