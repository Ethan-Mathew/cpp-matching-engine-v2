#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/Side.hpp"

#include "MemoryPool.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace lob::core;

constexpr std::size_t initialSlabSize = 100;

class MemoryPoolTest : public testing::Test
{
protected:
    MemoryPoolTest()
        : mp_{initialSlabSize}
    {
    }

    static RestingOrder* allocate_order(MemoryPool& mp,
                                        lob::OrderID id,
                                        lob::Quantity qty,
                                        RestingLifetime lifetime = RestingLifetime::GTC,
                                        lob::Side side = lob::Side::BUY)
    {
        return mp.allocate(id, qty, lifetime, side);
    }

    MemoryPool mp_;
};

TEST_F(MemoryPoolTest, ConstructionSanityChecks)
{
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
    EXPECT_EQ(mp_.get_currently_allocated(), 0);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
}

TEST_F(MemoryPoolTest, SingleAllocationWorks)
{
    RestingOrder* order1 = allocate_order(mp_, 1, 1, RestingLifetime::GTC, lob::Side::BUY);

    ASSERT_NE(order1, nullptr);
    EXPECT_EQ(order1->id_, 1);
    EXPECT_EQ(order1->quantity_, 1);
    EXPECT_EQ(order1->lifetime_, RestingLifetime::GTC);
    EXPECT_EQ(order1->side_, lob::Side::BUY);

    EXPECT_EQ(mp_.get_currently_allocated(), 1);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, SingleDeallocationWorks)
{
    RestingOrder* order1 = allocate_order(mp_, 1, 1, RestingLifetime::GTC, lob::Side::SELL);

    ASSERT_EQ(mp_.get_currently_allocated(), 1);

    mp_.deallocate(order1);

    EXPECT_EQ(mp_.get_currently_allocated(), 0);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, MultipleAllocationsConstructDistinctLiveObjects)
{
    std::vector<RestingOrder*> orders;
    orders.reserve(initialSlabSize);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        orders.push_back(allocate_order(mp_,
                                        static_cast<lob::OrderID>(i),
                                        static_cast<lob::Quantity>(i),
                                        RestingLifetime::GTC,
                                        lob::Side::BUY));
    }

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        ASSERT_NE(orders[i - 1], nullptr);
        EXPECT_EQ(orders[i - 1]->id_, i);
        EXPECT_EQ(orders[i - 1]->quantity_, i);
        EXPECT_EQ(orders[i - 1]->lifetime_, RestingLifetime::GTC);
        EXPECT_EQ(orders[i - 1]->side_, lob::Side::BUY);
    }

    for (std::uint64_t i = 1; i < initialSlabSize; ++i)
    {
        EXPECT_NE(orders[i - 1], orders[i]);
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, MultipleDeallocationsReturnPoolToEmptyLiveUsage)
{
    std::vector<RestingOrder*> orders;
    orders.reserve(initialSlabSize);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        orders.push_back(allocate_order(mp_,
                                        static_cast<lob::OrderID>(i),
                                        static_cast<lob::Quantity>(i),
                                        RestingLifetime::GTC,
                                        lob::Side::SELL));
    }

    for (RestingOrder* order : orders)
    {
        mp_.deallocate(order);
    }

    EXPECT_EQ(mp_.get_currently_allocated(), 0);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, FreedBlocksAreReusedWithoutGrowingPool)
{
    std::vector<RestingOrder*> firstRound;
    firstRound.reserve(initialSlabSize);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        firstRound.push_back(allocate_order(mp_,
                                            static_cast<lob::OrderID>(i),
                                            static_cast<lob::Quantity>(i),
                                            RestingLifetime::GTC,
                                            lob::Side::BUY));
    }

    std::unordered_set<RestingOrder*> originalAddresses(firstRound.begin(), firstRound.end());

    for (RestingOrder* order : firstRound)
    {
        mp_.deallocate(order);
    }

    EXPECT_EQ(mp_.get_currently_allocated(), 0);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);

    std::vector<RestingOrder*> secondRound;
    secondRound.reserve(initialSlabSize);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        secondRound.push_back(allocate_order(mp_,
                                             static_cast<lob::OrderID>(i),
                                             static_cast<lob::Quantity>(i),
                                             RestingLifetime::GTC,
                                             lob::Side::SELL));
    }

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        ASSERT_NE(secondRound[i - 1], nullptr);
        EXPECT_EQ(secondRound[i - 1]->id_, i);
        EXPECT_EQ(secondRound[i - 1]->quantity_, i);
        EXPECT_EQ(secondRound[i - 1]->lifetime_, RestingLifetime::GTC);
        EXPECT_EQ(secondRound[i - 1]->side_, lob::Side::SELL);
        EXPECT_TRUE(originalAddresses.contains(secondRound[i - 1]));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, PoolGrowsByNewSlabWhenCapacityIsExceeded)
{
    std::vector<RestingOrder*> orders;
    orders.reserve(initialSlabSize * 2 + 1);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        orders.push_back(allocate_order(mp_,
                                        static_cast<lob::OrderID>(i),
                                        static_cast<lob::Quantity>(i),
                                        RestingLifetime::GTC,
                                        lob::Side::BUY));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);

    orders.push_back(allocate_order(mp_,
                                    static_cast<lob::OrderID>(initialSlabSize + 1),
                                    static_cast<lob::Quantity>(initialSlabSize + 1),
                                    RestingLifetime::GTC,
                                    lob::Side::BUY));

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize + 1);
    EXPECT_EQ(mp_.get_num_slabs(), 2);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize * 2);

    for (std::uint64_t i = initialSlabSize + 2; i <= initialSlabSize * 2; ++i)
    {
        orders.push_back(allocate_order(mp_,
                                        static_cast<lob::OrderID>(i),
                                        static_cast<lob::Quantity>(i),
                                        RestingLifetime::GTC,
                                        lob::Side::BUY));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize * 2);
    EXPECT_EQ(mp_.get_num_slabs(), 2);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize * 2);

    orders.push_back(allocate_order(mp_,
                                    static_cast<lob::OrderID>(initialSlabSize * 2 + 1),
                                    static_cast<lob::Quantity>(initialSlabSize * 2 + 1),
                                    RestingLifetime::GTC,
                                    lob::Side::BUY));

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize * 2 + 1);
    EXPECT_EQ(mp_.get_num_slabs(), 3);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize * 4);
}

TEST_F(MemoryPoolTest, ReusesFreedBlocksBeforeGrowingPool)
{
    std::vector<RestingOrder*> initialOrders;
    initialOrders.reserve(initialSlabSize);

    for (std::uint64_t i = 1; i <= initialSlabSize; ++i)
    {
        initialOrders.push_back(allocate_order(mp_,
                                               static_cast<lob::OrderID>(i),
                                               static_cast<lob::Quantity>(i),
                                               RestingLifetime::GTC,
                                               lob::Side::BUY));
    }

    for (std::size_t i = 0; i < 10; ++i)
    {
        mp_.deallocate(initialOrders[i]);
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize - 10);
    EXPECT_EQ(mp_.get_num_slabs(), 1);

    std::vector<RestingOrder*> reusedOrders;
    reusedOrders.reserve(10);

    for (std::uint64_t i = 1; i <= 10; ++i)
    {
        reusedOrders.push_back(allocate_order(mp_,
                                              static_cast<lob::OrderID>(1000 + i),
                                              static_cast<lob::Quantity>(2000 + i),
                                              RestingLifetime::DAY,
                                              lob::Side::SELL));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);

    for (RestingOrder* order : reusedOrders)
    {
        ASSERT_NE(order, nullptr);
        EXPECT_EQ(order->lifetime_, RestingLifetime::DAY);
        EXPECT_EQ(order->side_, lob::Side::SELL);
    }
}