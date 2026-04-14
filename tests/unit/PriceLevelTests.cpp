#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/Side.hpp"

#include "PriceLevel.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"

using namespace lob::core;

using lob::OrderID;
using lob::Price;
using lob::Quantity;
using lob::Side;
using lob::Volume;

constexpr lob::Price defaultPrice = 100;

class PriceLevelTest : public testing::Test
{
protected:
    PriceLevelTest()
        : pl_{defaultPrice}
    {
    }

    static RestingOrder make_order(OrderID id,
                                   Quantity qty,
                                   RestingLifetime lifetime = RestingLifetime::GTC,
                                   Side side = Side::BUY)
    {
        return RestingOrder{id, qty, lifetime, side};
    }

    PriceLevel pl_;
};

TEST_F(PriceLevelTest, PriceLevelConstructsEmpty)
{
    EXPECT_TRUE(pl_.empty());
    EXPECT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.pop_front(), nullptr);
    EXPECT_EQ(pl_.get_total_volume(), 0);
    EXPECT_EQ(pl_.get_order_count(), 0);
}

TEST_F(PriceLevelTest, SinglePushBackWorks)
{
    RestingOrder order1 = make_order(1, 1);

    pl_.push_back(&order1);

    EXPECT_EQ(pl_.get_order_count(), 1);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_);
    EXPECT_EQ(pl_.front(), &order1);
}

TEST_F(PriceLevelTest, MultiPushBackWorks)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 3);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order2.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, SinglePopFrontWorks)
{
    RestingOrder order1 = make_order(1, 1);

    pl_.push_back(&order1);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 1);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_);

    RestingOrder* returnedOrder1 = pl_.pop_front();

    ASSERT_EQ(returnedOrder1, &order1);
    EXPECT_EQ(returnedOrder1->next_, nullptr);
    EXPECT_EQ(returnedOrder1->prev_, nullptr);
    EXPECT_EQ(returnedOrder1->level_, nullptr);

    EXPECT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, MultiPopFrontWorks)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 3);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order2.quantity_ + order3.quantity_);

    RestingOrder* returnedOrder1 = pl_.pop_front();

    ASSERT_EQ(returnedOrder1, &order1);
    EXPECT_EQ(returnedOrder1->next_, nullptr);
    EXPECT_EQ(returnedOrder1->prev_, nullptr);
    EXPECT_EQ(returnedOrder1->level_, nullptr);

    ASSERT_EQ(pl_.front(), &order2);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order2.quantity_ + order3.quantity_);

    RestingOrder* returnedOrder2 = pl_.pop_front();

    ASSERT_EQ(returnedOrder2, &order2);
    EXPECT_EQ(returnedOrder2->next_, nullptr);
    EXPECT_EQ(returnedOrder2->prev_, nullptr);
    EXPECT_EQ(returnedOrder2->level_, nullptr);

    RestingOrder* returnedOrder3 = pl_.pop_front();

    ASSERT_EQ(returnedOrder3, &order3);
    EXPECT_EQ(returnedOrder3->next_, nullptr);
    EXPECT_EQ(returnedOrder3->prev_, nullptr);
    EXPECT_EQ(returnedOrder3->level_, nullptr);

    EXPECT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, RemoveOnlyOrder)
{
    RestingOrder order1 = make_order(1, 1);

    pl_.push_back(&order1);

    auto removeResult1 = pl_.remove_order(&order1);

    ASSERT_EQ(removeResult1, PriceLevel::RemoveOrderResult::EMPTY);

    EXPECT_EQ(order1.next_, nullptr);
    EXPECT_EQ(order1.prev_, nullptr);
    EXPECT_EQ(order1.level_, nullptr);

    EXPECT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, RemoveHeadOrder)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    auto removeResult1 = pl_.remove_order(&order1);

    ASSERT_EQ(removeResult1, PriceLevel::RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order1.next_, nullptr);
    EXPECT_EQ(order1.prev_, nullptr);
    EXPECT_EQ(order1.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order2);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order2.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, RemoveTailOrder)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    auto removeResult1 = pl_.remove_order(&order3);

    ASSERT_EQ(removeResult1, PriceLevel::RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order3.next_, nullptr);
    EXPECT_EQ(order3.prev_, nullptr);
    EXPECT_EQ(order3.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order2.quantity_);
}

TEST_F(PriceLevelTest, RemoveMiddleOrder)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    auto removeResult1 = pl_.remove_order(&order2);

    ASSERT_EQ(removeResult1, PriceLevel::RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order2.next_, nullptr);
    EXPECT_EQ(order2.prev_, nullptr);
    EXPECT_EQ(order2.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, MixedOrderRemovals)
{
    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    auto removeResult1 = pl_.remove_order(&order2);

    ASSERT_EQ(removeResult1, PriceLevel::RemoveOrderResult::NON_EMPTY);
    ASSERT_EQ(pl_.front(), &order1);

    RestingOrder* popped = pl_.pop_front();
    ASSERT_EQ(popped, &order1);
    EXPECT_EQ(popped->next_, nullptr);
    EXPECT_EQ(popped->prev_, nullptr);
    EXPECT_EQ(popped->level_, nullptr);

    auto removeResult2 = pl_.remove_order(&order3);

    ASSERT_EQ(removeResult2, PriceLevel::RemoveOrderResult::EMPTY);

    EXPECT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}