#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Results.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

using namespace lob;

constexpr std::size_t initialSlabSize = 10;
constexpr Price defaultPrice = 10000;

class OBModifyOrderTest : public testing::Test
{
protected:
    OBModifyOrderTest()
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

    void expect_empty_book()
    {
        EXPECT_EQ(ob_.get_num_orders(), 0);
        EXPECT_EQ(ob_.get_num_levels_bids(), 0);
        EXPECT_EQ(ob_.get_num_levels_asks(), 0);
        EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 0);
    }

    OrderBook ob_;
};

TEST_F(OBModifyOrderTest, ModifyMissingOrderReturnsNotFound)
{
    ModificationResult result = ob_.modify_order(make_modify(42, 5, defaultPrice));

    EXPECT_EQ(result.status_, ModificationStatus::NOT_FOUND);
    EXPECT_EQ(result.originalQuantity_, 0);
    EXPECT_FALSE(result.resubmissionResult_.has_value());

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifyToZeroQuantityBecomesCancel)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY));

    ASSERT_EQ(ob_.get_num_orders(), 1);
    ASSERT_EQ(ob_.get_num_levels_bids(), 1);
    ASSERT_EQ(ob_.get_memory_pool_curr_alloc(), 1);

    ModificationResult result = ob_.modify_order(make_modify(1, 0, defaultPrice + 10));

    EXPECT_EQ(result.status_, ModificationStatus::CANCELED);
    EXPECT_EQ(result.originalQuantity_, 5);
    EXPECT_FALSE(result.resubmissionResult_.has_value());

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifyBuyOrderToNewNonCrossingPriceResubmitsAndRests)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY));

    ModificationResult result = ob_.modify_order(make_modify(1, 7, defaultPrice + 20));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 5);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::RESTING);
    EXPECT_EQ(sub.quantityRequested_, 7);
    EXPECT_EQ(sub.quantityFilled_, 0);
    EXPECT_EQ(sub.get_quantity_remaining(), 7);
    EXPECT_TRUE(sub.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice + 20, Side::BUY));
    EXPECT_FALSE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice + 20, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice + 20, Side::BUY), 7);
}

TEST_F(OBModifyOrderTest, ModifySellOrderToNewNonCrossingPriceResubmitsAndRests)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 4, Side::SELL));

    ModificationResult result = ob_.modify_order(make_modify(1, 6, defaultPrice + 15));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 4);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::RESTING);
    EXPECT_EQ(sub.quantityRequested_, 6);
    EXPECT_EQ(sub.quantityFilled_, 0);
    EXPECT_EQ(sub.get_quantity_remaining(), 6);
    EXPECT_TRUE(sub.executions_.empty());

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice + 15, Side::SELL));
    EXPECT_FALSE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice + 15, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice + 15, Side::SELL), 6);
}

TEST_F(OBModifyOrderTest, ModifyBuyOrderCanBecomeAggressiveAndFullyFill)
{
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 4, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(1, defaultPrice - 10, 3, Side::BUY, TimeInForce::GTC));

    ModificationResult result = ob_.modify_order(make_modify(1, 4, defaultPrice));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 3);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::FILLED);
    EXPECT_EQ(sub.quantityRequested_, 4);
    EXPECT_EQ(sub.quantityFilled_, 4);
    EXPECT_EQ(sub.get_quantity_remaining(), 0);
    ASSERT_EQ(sub.executions_.size(), 1);
    EXPECT_EQ(sub.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(sub.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(sub.executions_[0].executedQuantity_, 4);

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifySellOrderCanBecomeAggressiveAndFullyFill)
{
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 5, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(1, defaultPrice + 10, 3, Side::SELL, TimeInForce::GTC));

    ModificationResult result = ob_.modify_order(make_modify(1, 5, defaultPrice));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 3);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::FILLED);
    EXPECT_EQ(sub.quantityRequested_, 5);
    EXPECT_EQ(sub.quantityFilled_, 5);
    EXPECT_EQ(sub.get_quantity_remaining(), 0);
    ASSERT_EQ(sub.executions_.size(), 1);
    EXPECT_EQ(sub.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(sub.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(sub.executions_[0].executedQuantity_, 5);

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifyBuyOrderCanPartiallyFillThenRestRemainder)
{
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(1, defaultPrice - 10, 3, Side::BUY, TimeInForce::GTC));

    ModificationResult result = ob_.modify_order(make_modify(1, 5, defaultPrice));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 3);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::PARTIALLY_FILLED_RESTING);
    EXPECT_EQ(sub.quantityRequested_, 5);
    EXPECT_EQ(sub.quantityFilled_, 2);
    EXPECT_EQ(sub.get_quantity_remaining(), 3);
    ASSERT_EQ(sub.executions_.size(), 1);
    EXPECT_EQ(sub.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(sub.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(sub.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::BUY), 3);
}

TEST_F(OBModifyOrderTest, ModifySellOrderCanPartiallyFillThenRestRemainder)
{
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 2, Side::BUY, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(1, defaultPrice + 10, 4, Side::SELL, TimeInForce::GTC));

    ModificationResult result = ob_.modify_order(make_modify(1, 6, defaultPrice));

    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    EXPECT_EQ(result.originalQuantity_, 4);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    const SubmissionResult& sub = *result.resubmissionResult_;
    EXPECT_EQ(sub.status_, SubmitStatus::PARTIALLY_FILLED_RESTING);
    EXPECT_EQ(sub.quantityRequested_, 6);
    EXPECT_EQ(sub.quantityFilled_, 2);
    EXPECT_EQ(sub.get_quantity_remaining(), 4);
    ASSERT_EQ(sub.executions_.size(), 1);
    EXPECT_EQ(sub.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(sub.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(sub.executions_[0].executedQuantity_, 2);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 4);
}

TEST_F(OBModifyOrderTest, ModifyPreservesSideWhenResubmitting)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 3, Side::BUY));

    ModificationResult result = ob_.modify_order(make_modify(1, 6, defaultPrice + 5));

    ASSERT_TRUE(result.resubmissionResult_.has_value());
    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);

    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice + 5, Side::BUY));
    EXPECT_FALSE(ob_.check_level_exists(defaultPrice + 5, Side::SELL));
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice + 5, Side::BUY), 6);
}

TEST_F(OBModifyOrderTest, ModifyPreservesDayLifetimeForSessionEndPruning)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY, TimeInForce::DAY));

    ModificationResult result = ob_.modify_order(make_modify(1, 7, defaultPrice + 10));

    ASSERT_TRUE(result.resubmissionResult_.has_value());
    EXPECT_EQ(result.status_, ModificationStatus::RESUBMITTED);

    DayOrderPruneResult pruneResult = ob_.on_session_end();

    EXPECT_EQ(pruneResult.ordersPruned, 1);
    EXPECT_EQ(pruneResult.sharesErased, 7);
    EXPECT_EQ(pruneResult.priceLevelsErased, 1);

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifyLosesQueuePriority)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 3, Side::SELL, TimeInForce::GTC));

    ModificationResult modifyResult = ob_.modify_order(make_modify(1, 2, defaultPrice));

    ASSERT_TRUE(modifyResult.resubmissionResult_.has_value());
    EXPECT_EQ(modifyResult.status_, ModificationStatus::RESUBMITTED);

    SubmissionResult takerResult =
        ob_.submit_limit_order(make_limit_order(3, defaultPrice, 4, Side::BUY, TimeInForce::IOC));
    
    EXPECT_EQ(takerResult.status_, SubmitStatus::FILLED);
    EXPECT_EQ(takerResult.quantityRequested_, 4);
    EXPECT_EQ(takerResult.quantityFilled_, 4);
    EXPECT_EQ(takerResult.get_quantity_remaining(), 0);
    ASSERT_EQ(takerResult.executions_.size(), 2);
    
    EXPECT_EQ(takerResult.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(takerResult.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(takerResult.executions_[0].executedQuantity_, 3);

    EXPECT_EQ(takerResult.executions_[1].makerOrderID_, 1);
    EXPECT_EQ(takerResult.executions_[1].makerPrice_, defaultPrice);
    EXPECT_EQ(takerResult.executions_[1].executedQuantity_, 1);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 1);
}

TEST_F(OBModifyOrderTest, ModifyPreservesGTCLifetimeThroughSessionEnd)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY, TimeInForce::GTC));

    ModificationResult result = ob_.modify_order(make_modify(1, 7, defaultPrice + 10));

    ASSERT_EQ(result.status_, ModificationStatus::RESUBMITTED);
    ASSERT_TRUE(result.resubmissionResult_.has_value());

    DayOrderPruneResult pruneResult = ob_.on_session_end();

    EXPECT_EQ(pruneResult.ordersPruned, 0);
    EXPECT_EQ(pruneResult.sharesErased, 0);
    EXPECT_EQ(pruneResult.priceLevelsErased, 0);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice + 10, Side::BUY));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice + 10, Side::BUY), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice + 10, Side::BUY), 7);
}

TEST_F(OBModifyOrderTest, ModifyAfterSessionEndPrunedDayOrderReturnsNotFound)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 5, Side::BUY, TimeInForce::DAY));

    DayOrderPruneResult pruneResult = ob_.on_session_end();

    ASSERT_EQ(pruneResult.ordersPruned, 1);
    ASSERT_EQ(pruneResult.sharesErased, 5);
    ASSERT_EQ(pruneResult.priceLevelsErased, 1);

    ModificationResult result = ob_.modify_order(make_modify(1, 8, defaultPrice + 5));

    EXPECT_EQ(result.status_, ModificationStatus::NOT_FOUND);
    EXPECT_EQ(result.originalQuantity_, 0);
    EXPECT_FALSE(result.resubmissionResult_.has_value());

    expect_empty_book();
}

TEST_F(OBModifyOrderTest, ModifySamePriceAndSameQuantityStillLosesQueuePriority)
{
    ob_.submit_limit_order(make_limit_order(1, defaultPrice, 2, Side::SELL, TimeInForce::GTC));
    ob_.submit_limit_order(make_limit_order(2, defaultPrice, 3, Side::SELL, TimeInForce::GTC));

    ModificationResult modifyResult = ob_.modify_order(make_modify(1, 2, defaultPrice));

    ASSERT_EQ(modifyResult.status_, ModificationStatus::RESUBMITTED);
    ASSERT_TRUE(modifyResult.resubmissionResult_.has_value());

    SubmissionResult takerResult =
        ob_.submit_limit_order(make_limit_order(3, defaultPrice, 4, Side::BUY, TimeInForce::IOC));

    EXPECT_EQ(takerResult.status_, SubmitStatus::FILLED);
    EXPECT_EQ(takerResult.quantityRequested_, 4);
    EXPECT_EQ(takerResult.quantityFilled_, 4);
    EXPECT_EQ(takerResult.get_quantity_remaining(), 0);
    ASSERT_EQ(takerResult.executions_.size(), 2u);

    EXPECT_EQ(takerResult.executions_[0].makerOrderID_, 2);
    EXPECT_EQ(takerResult.executions_[0].makerPrice_, defaultPrice);
    EXPECT_EQ(takerResult.executions_[0].executedQuantity_, 3);

    EXPECT_EQ(takerResult.executions_[1].makerOrderID_, 1);
    EXPECT_EQ(takerResult.executions_[1].makerPrice_, defaultPrice);
    EXPECT_EQ(takerResult.executions_[1].executedQuantity_, 1);

    EXPECT_EQ(ob_.get_num_orders(), 1);
    EXPECT_EQ(ob_.get_num_levels_asks(), 1);
    EXPECT_EQ(ob_.get_num_levels_bids(), 0);
    EXPECT_EQ(ob_.get_memory_pool_curr_alloc(), 1);
    EXPECT_TRUE(ob_.check_level_exists(defaultPrice, Side::SELL));
    EXPECT_EQ(ob_.get_num_orders_at_level(defaultPrice, Side::SELL), 1);
    EXPECT_EQ(ob_.get_num_shares_at_level(defaultPrice, Side::SELL), 1);
}