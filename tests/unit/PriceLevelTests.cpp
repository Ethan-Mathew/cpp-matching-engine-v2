#include <gtest/gtest.h>

#include "lob/Aliases.hpp"

#include "PriceLevel.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"

using namespace lob::core;

class PriceLevelTest : public testing::Test
{
protected:
    PriceLevelTest()
        : pl_{PriceLevel{100}}
    {
    }

    PriceLevel pl_;
};

TEST_F(PriceLevelTest, PriceLevelConstructsEmpty)
{
    ASSERT_TRUE(pl_.empty());
    ASSERT_EQ(pl_.front(), nullptr);
    ASSERT_EQ(pl_.pop_front(), nullptr);
    ASSERT_EQ(pl_.get_total_volume(), 0);
    ASSERT_EQ(pl_.get_order_count(), 0);
}

TEST_F(PriceLevelTest, SinglePushBackWorks)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    
    pl_.push_back(&order1);

    EXPECT_EQ(pl_.get_order_count(), 1);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_);
    EXPECT_EQ(pl_.front(), &order1);
}

TEST_F(PriceLevelTest, MultiPushBackWorks)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    
    pl_.push_back(&order1);

    EXPECT_EQ(pl_.get_order_count(), 1);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_);
    EXPECT_EQ(pl_.front(), &order1);

    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

    pl_.push_back(&order2);
    pl_.push_back(&order3);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 3);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order2.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, SinglePopFrontWorks)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    
    pl_.push_back(&order1);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 1);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_);

    RestingOrder* returnedOrder1 = pl_.pop_front();

    ASSERT_EQ(returnedOrder1, &order1);
    EXPECT_EQ(returnedOrder1->next_, nullptr);
    EXPECT_EQ(returnedOrder1->prev_, nullptr);
    EXPECT_EQ(returnedOrder1->level_, nullptr);

    ASSERT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, MultiPopFrontWorks)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

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

    ASSERT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, RemoveOnlyOrder)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    
    pl_.push_back(&order1);

    RemoveOrderResult removeResult1 = pl_.remove_order(&order1);

    ASSERT_EQ(removeResult1, RemoveOrderResult::EMPTY);

    EXPECT_EQ(order1.next_, nullptr);
    EXPECT_EQ(order1.prev_, nullptr);
    EXPECT_EQ(order1.level_, nullptr);

    ASSERT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

TEST_F(PriceLevelTest, RemoveHeadOrder)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    RemoveOrderResult removeResult1 = pl_.remove_order(&order1);

    ASSERT_EQ(removeResult1, RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order1.next_, nullptr);
    EXPECT_EQ(order1.prev_, nullptr);
    EXPECT_EQ(order1.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order2);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order2.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, RemoveTailOrder)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    RemoveOrderResult removeResult1 = pl_.remove_order(&order3);

    ASSERT_EQ(removeResult1, RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order3.next_, nullptr);
    EXPECT_EQ(order3.prev_, nullptr);
    EXPECT_EQ(order3.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order2.quantity_);
}

TEST_F(PriceLevelTest, RemoveMiddleOrder)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    RemoveOrderResult removeResult1 = pl_.remove_order(&order2);

    ASSERT_EQ(removeResult1, RemoveOrderResult::NON_EMPTY);

    EXPECT_EQ(order2.next_, nullptr);
    EXPECT_EQ(order2.prev_, nullptr);
    EXPECT_EQ(order2.level_, nullptr);

    ASSERT_EQ(pl_.front(), &order1);
    EXPECT_EQ(pl_.get_order_count(), 2);
    EXPECT_EQ(pl_.get_total_volume(), order1.quantity_ + order3.quantity_);
}

TEST_F(PriceLevelTest, MixedOrderRemovals)
{
    RestingOrder order1{1, 1, RestingLifetime::GTC};
    RestingOrder order2{2, 2, RestingLifetime::GTC};
    RestingOrder order3{3, 3, RestingLifetime::GTC};

    pl_.push_back(&order1);
    pl_.push_back(&order2);
    pl_.push_back(&order3);

    RemoveOrderResult removeResult1 = pl_.remove_order(&order2);

    ASSERT_EQ(removeResult1, RemoveOrderResult::NON_EMPTY);

    ASSERT_EQ(pl_.front(), &order1);
    
    pl_.pop_front();

    RemoveOrderResult removeResult2 = pl_.remove_order(&order3);

    ASSERT_EQ(removeResult2, RemoveOrderResult::EMPTY);

    ASSERT_EQ(pl_.front(), nullptr);
    EXPECT_EQ(pl_.get_order_count(), 0);
    EXPECT_EQ(pl_.get_total_volume(), 0);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}