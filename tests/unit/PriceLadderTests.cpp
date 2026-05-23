#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "PriceLadder.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"
#include "lob/Aliases.hpp"
#include "lob/Results.hpp"
#include "lob/Side.hpp"

using namespace lob::core;

using lob::DayOrderPruneResult;
using lob::OrderID;
using lob::Price;
using lob::Quantity;
using lob::Side;
using lob::Volume;

constexpr Price minPrice = 95;
constexpr Price maxPrice = 105;
constexpr Price middlePrice = 100;

class PriceLadderTest : public testing::Test {
  protected:
    static RestingOrder make_order(OrderID id, Quantity qty,
                                   RestingLifetime lifetime = RestingLifetime::GTC,
                                   Side side = Side::BUY) {
        return RestingOrder{id, qty, lifetime, side};
    }
};

TEST_F(PriceLadderTest, BuyLadderConstructsEmpty) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    EXPECT_EQ(ladder.get_side(), Side::BUY);
    EXPECT_FALSE(ladder.get_best_price().has_value());
    EXPECT_EQ(ladder.get_num_non_empty_levels(), 0);
    EXPECT_TRUE(ladder.empty());
}

TEST_F(PriceLadderTest, SellLadderConstructsEmpty) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    EXPECT_EQ(ladder.get_side(), Side::SELL);
    EXPECT_FALSE(ladder.get_best_price().has_value());
    EXPECT_EQ(ladder.get_num_non_empty_levels(), 0);
    EXPECT_TRUE(ladder.empty());
}

TEST_F(PriceLadderTest, LevelLookupReturnsUsableLevelAtPrice) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    PriceLevel* level = ladder.get_level_at_price(middlePrice);

    ASSERT_NE(level, nullptr);
    EXPECT_TRUE(level->empty());
}

TEST_F(PriceLadderTest, LiquidityAtPriceReflectsUnderlyingLevelState) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 10);

    EXPECT_FALSE(ladder.has_liquidity_at_price(middlePrice));

    PriceLevel* level = ladder.get_level_at_price(middlePrice);
    level->push_back(&order1);

    EXPECT_TRUE(ladder.has_liquidity_at_price(middlePrice));
}

TEST_F(PriceLadderTest, CountsNonEmptyLevels) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 2);
    RestingOrder order3 = make_order(3, 3);

    ladder.get_level_at_price(97)->push_back(&order1);
    ladder.get_level_at_price(100)->push_back(&order2);
    ladder.get_level_at_price(103)->push_back(&order3);

    EXPECT_EQ(ladder.get_num_non_empty_levels(), 3);

    ladder.get_level_at_price(100)->pop_front();

    EXPECT_EQ(ladder.get_num_non_empty_levels(), 2);
}

TEST_F(PriceLadderTest, SetBestPriceStoresConfiguredPrice) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    ladder.set_best_price(102);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), 102);
}

TEST_F(PriceLadderTest, BuyLadderUpdatesBestPriceDownward) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 1);
    RestingOrder order2 = make_order(2, 1);
    RestingOrder order3 = make_order(3, 1);

    ladder.get_level_at_price(98)->push_back(&order1);
    ladder.get_level_at_price(101)->push_back(&order2);
    ladder.get_level_at_price(104)->push_back(&order3);

    ladder.set_best_price(104);

    ladder.get_level_at_price(104)->pop_front();
    ladder.update_best_price_from_given(104);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), 101);
}

TEST_F(PriceLadderTest, SellLadderUpdatesBestPriceUpward) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    RestingOrder order1 = make_order(1, 1, RestingLifetime::GTC, Side::SELL);
    RestingOrder order2 = make_order(2, 1, RestingLifetime::GTC, Side::SELL);
    RestingOrder order3 = make_order(3, 1, RestingLifetime::GTC, Side::SELL);

    ladder.get_level_at_price(96)->push_back(&order1);
    ladder.get_level_at_price(99)->push_back(&order2);
    ladder.get_level_at_price(103)->push_back(&order3);

    ladder.set_best_price(96);

    ladder.get_level_at_price(96)->pop_front();
    ladder.update_best_price_from_given(96);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), 99);
}

TEST_F(PriceLadderTest, BuyLadderCanFindLowestConfiguredPrice) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 1);

    ladder.get_level_at_price(minPrice)->push_back(&order1);
    ladder.set_best_price(minPrice + 1);

    ladder.update_best_price_from_given(minPrice + 1);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), minPrice);
}

TEST_F(PriceLadderTest, SellLadderCanFindHighestConfiguredPrice) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    RestingOrder order1 = make_order(1, 1, RestingLifetime::GTC, Side::SELL);

    ladder.get_level_at_price(maxPrice)->push_back(&order1);
    ladder.set_best_price(maxPrice - 1);

    ladder.update_best_price_from_given(maxPrice - 1);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), maxPrice);
}

TEST_F(PriceLadderTest, UpdatingBestPriceWithNoLiquidityClearsBestPrice) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    ladder.set_best_price(101);
    ladder.update_best_price_from_given(101);

    EXPECT_FALSE(ladder.get_best_price().has_value());
    EXPECT_TRUE(ladder.empty());
}

TEST_F(PriceLadderTest, BuyLadderMarketableLiquidityReturnsTrueWhenEnoughLiquidityExists) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 2);
    RestingOrder order2 = make_order(2, 3);
    RestingOrder order3 = make_order(3, 4);

    ladder.get_level_at_price(103)->push_back(&order1);
    ladder.get_level_at_price(101)->push_back(&order2);
    ladder.get_level_at_price(99)->push_back(&order3);

    ladder.set_best_price(103);

    EXPECT_TRUE(ladder.has_sufficient_marketable_liquidity(101, 5));
    EXPECT_TRUE(ladder.has_sufficient_marketable_liquidity(99, 9));
}

TEST_F(PriceLadderTest, BuyLadderMarketableLiquidityReturnsFalseWhenInsufficientOrNotCrossing) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder order1 = make_order(1, 2);
    RestingOrder order2 = make_order(2, 3);

    ladder.get_level_at_price(103)->push_back(&order1);
    ladder.get_level_at_price(101)->push_back(&order2);

    ladder.set_best_price(103);

    EXPECT_FALSE(ladder.has_sufficient_marketable_liquidity(101, 6));
    EXPECT_FALSE(ladder.has_sufficient_marketable_liquidity(104, 1));
}

TEST_F(PriceLadderTest, SellLadderMarketableLiquidityReturnsTrueWhenEnoughLiquidityExists) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    RestingOrder order1 = make_order(1, 2, RestingLifetime::GTC, Side::SELL);
    RestingOrder order2 = make_order(2, 3, RestingLifetime::GTC, Side::SELL);
    RestingOrder order3 = make_order(3, 4, RestingLifetime::GTC, Side::SELL);

    ladder.get_level_at_price(97)->push_back(&order1);
    ladder.get_level_at_price(99)->push_back(&order2);
    ladder.get_level_at_price(101)->push_back(&order3);

    ladder.set_best_price(97);

    EXPECT_TRUE(ladder.has_sufficient_marketable_liquidity(99, 5));
    EXPECT_TRUE(ladder.has_sufficient_marketable_liquidity(101, 9));
}

TEST_F(PriceLadderTest, SellLadderMarketableLiquidityReturnsFalseWhenInsufficientOrNotCrossing) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    RestingOrder order1 = make_order(1, 2, RestingLifetime::GTC, Side::SELL);
    RestingOrder order2 = make_order(2, 3, RestingLifetime::GTC, Side::SELL);

    ladder.get_level_at_price(97)->push_back(&order1);
    ladder.get_level_at_price(99)->push_back(&order2);

    ladder.set_best_price(97);

    EXPECT_FALSE(ladder.has_sufficient_marketable_liquidity(99, 6));
    EXPECT_FALSE(ladder.has_sufficient_marketable_liquidity(96, 1));
}

TEST_F(PriceLadderTest, EmptyLadderHasNoSufficientMarketableLiquidity) {
    PriceLadder buyLadder{minPrice, maxPrice, Side::BUY};
    PriceLadder sellLadder{minPrice, maxPrice, Side::SELL};

    EXPECT_FALSE(buyLadder.has_sufficient_marketable_liquidity(100, 1));
    EXPECT_FALSE(sellLadder.has_sufficient_marketable_liquidity(100, 1));
}

TEST_F(PriceLadderTest, PruneDayOrdersRemovesOnlyDayOrdersAndAccumulatesStats) {
    PriceLadder ladder{minPrice, maxPrice, Side::SELL};

    RestingOrder dayOrder1 = make_order(1, 2, RestingLifetime::DAY, Side::SELL);

    RestingOrder gtcOrder = make_order(2, 3, RestingLifetime::GTC, Side::SELL);

    RestingOrder dayOrder2 = make_order(3, 4, RestingLifetime::DAY, Side::SELL);

    ladder.get_level_at_price(97)->push_back(&dayOrder1);
    ladder.get_level_at_price(99)->push_back(&gtcOrder);
    ladder.get_level_at_price(101)->push_back(&dayOrder2);

    ladder.set_best_price(97);

    DayOrderPruneResult result{};
    std::vector<OrderID> prunedOrderIds;

    ladder.prune_day_orders(result,
                            [&](RestingOrder* order) { prunedOrderIds.push_back(order->id_); });

    std::sort(prunedOrderIds.begin(), prunedOrderIds.end());

    ASSERT_EQ(prunedOrderIds.size(), 2);
    EXPECT_EQ(prunedOrderIds[0], 1);
    EXPECT_EQ(prunedOrderIds[1], 3);

    EXPECT_EQ(result.ordersPruned, 2);
    EXPECT_EQ(result.sharesErased, 6);
    EXPECT_EQ(result.priceLevelsErased, 2);

    EXPECT_FALSE(ladder.has_liquidity_at_price(97));
    EXPECT_TRUE(ladder.has_liquidity_at_price(99));
    EXPECT_FALSE(ladder.has_liquidity_at_price(101));

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), 99);
}

TEST_F(PriceLadderTest, PruneDayOrdersLeavesBestPriceAloneWhenBestLevelRemainsNonEmpty) {
    PriceLadder ladder{minPrice, maxPrice, Side::BUY};

    RestingOrder bestGtcOrder = make_order(1, 5, RestingLifetime::GTC, Side::BUY);

    RestingOrder dayOrder = make_order(2, 2, RestingLifetime::DAY, Side::BUY);

    ladder.get_level_at_price(103)->push_back(&bestGtcOrder);
    ladder.get_level_at_price(101)->push_back(&dayOrder);

    ladder.set_best_price(103);

    DayOrderPruneResult result{};

    ladder.prune_day_orders(result, [](RestingOrder*) {});

    EXPECT_EQ(result.ordersPruned, 1);
    EXPECT_EQ(result.sharesErased, 2);
    EXPECT_EQ(result.priceLevelsErased, 1);

    ASSERT_TRUE(ladder.get_best_price().has_value());
    EXPECT_EQ(*ladder.get_best_price(), 103);
}