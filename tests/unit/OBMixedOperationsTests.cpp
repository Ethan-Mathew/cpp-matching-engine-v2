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

class OBInvariantTest : public testing::Test
{
protected:
    OBInvariantTest()
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

    static ModifyOrderRequest make_modify(OrderID id,
                                          Quantity newQty,
                                          Price newPrice)
    {
        return ModifyOrderRequest{id, newQty, newPrice};
    }

    static CancelOrderRequest make_cancel(OrderID id)
    {
        return CancelOrderRequest{id};
    }

    OrderBook ob_;
};

TEST_F(OBInvariantTest, MixedOperationsLeaveConsistentObservableState)
{
    ob_.submit_limit_order(make_limit_order(1,  9990, 5, Side::BUY,  TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(2,  9980, 4, Side::BUY,  TimeInForce::DAY));
    ob_.submit_limit_order(make_limit_order(3, 10010, 6, Side::SELL, TimeInForce::GTC));

    ModificationResult modifyResult = ob_.modify_order(make_modify(1, 3, 10010));
    ASSERT_EQ(modifyResult.status_, ModificationStatus::RESUBMITTED);
    ASSERT_TRUE(modifyResult.resubmissionResult_.has_value());
    EXPECT_EQ(modifyResult.resubmissionResult_->status_, SubmitStatus::FILLED);
    EXPECT_EQ(modifyResult.resubmissionResult_->quantityFilled_, 3);

    CancelResult cancelResult = ob_.cancel_order(make_cancel(2));
    ASSERT_EQ(cancelResult.status_, CancelStatus::CANCELED);
    EXPECT_EQ(cancelResult.quantityCancelled_, 4);

    DayOrderPruneResult pruneResult = ob_.on_session_end();
    EXPECT_EQ(pruneResult.ordersPruned, 0);
    EXPECT_EQ(pruneResult.sharesErased, 0);
    EXPECT_EQ(pruneResult.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(10010, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(10010, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(10010, Side::SELL), 3);
}

TEST_F(OBInvariantTest, PoolAccountingTracksSubmitModifyCancelAndPruneFlows)
{
    SubmissionResult submit1 =
        ob_.submit_limit_order(make_limit_order(1, 9995, 5, Side::BUY, TimeInForce::GTC));
    ASSERT_EQ(submit1.status_, SubmitStatus::RESTING);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    ModificationResult modifyResult = ob_.modify_order(make_modify(1, 7, 9990));
    ASSERT_EQ(modifyResult.status_, ModificationStatus::RESUBMITTED);
    ASSERT_TRUE(modifyResult.resubmissionResult_.has_value());
    EXPECT_EQ(modifyResult.resubmissionResult_->status_, SubmitStatus::RESTING);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    SubmissionResult submit2 =
        ob_.submit_limit_order(make_limit_order(2, 10020, 4, Side::SELL, TimeInForce::DAY));
    ASSERT_EQ(submit2.status_, SubmitStatus::RESTING);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 2);

    DayOrderPruneResult pruneResult = ob_.on_session_end();
    EXPECT_EQ(pruneResult.ordersPruned, 1);
    EXPECT_EQ(pruneResult.sharesErased, 4);
    EXPECT_EQ(pruneResult.priceLevelsErased, 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    CancelResult cancelResult = ob_.cancel_order(make_cancel(1));
    EXPECT_EQ(cancelResult.status_, CancelStatus::CANCELED);
    EXPECT_EQ(cancelResult.quantityCancelled_, 7);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);

    EXPECT_EQ(ob_.get_num_orders(), 0);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
}