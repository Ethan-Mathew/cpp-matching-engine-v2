#include <gtest/gtest.h>

#include "MemoryPool.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"

#include <cstddef>
#include <cstdint>

using namespace lob::core;

constexpr std::size_t initialSlabSize = 100;

class MemoryPoolTest : public testing::Test
{
protected:
    MemoryPoolTest()
        : mp_{initialSlabSize}
    {
    }

    MemoryPool mp_;
};

TEST_F(MemoryPoolTest, ConstructionSanityChecks)
{
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize);
    ASSERT_EQ(mp_.get_currently_allocated(), 0);
    ASSERT_EQ(mp_.get_num_slabs(), 1);
}

TEST_F(MemoryPoolTest, SingleAllocationWorks)
{
    RestingOrder* order1 = mp_.allocate(1, 1, RestingLifetime::GTC);

    EXPECT_EQ(order1->id_, 1);
    EXPECT_EQ(order1->quantity_, 1);
    EXPECT_EQ(order1->lifetime_, RestingLifetime::GTC);

    ASSERT_EQ(mp_.get_currently_allocated(), 1);
}

TEST_F(MemoryPoolTest, SingleDeallocationWorks)
{
    RestingOrder* order1 = mp_.allocate(1, 1, RestingLifetime::GTC);

    ASSERT_EQ(mp_.get_currently_allocated(), 1);

    mp_.deallocate(order1);

    ASSERT_EQ(mp_.get_currently_allocated(), 0);
}

TEST_F(MemoryPoolTest, MultipleAllocations)
{
    std::vector<RestingOrder*> orders;

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {
        EXPECT_EQ(orders[i - 1]->id_, i);
        EXPECT_EQ(orders[i - 1]->quantity_, i);
        EXPECT_EQ(orders[i - 1]->lifetime_, RestingLifetime::GTC);
    }

    ASSERT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    ASSERT_EQ(mp_.get_num_slabs(), 1);
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, VerifyAllocationDistinctness)
{
    std::vector<RestingOrder*> orders;

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    for (std::uint64_t i = 1; i < initialSlabSize; i++)
    {
        ASSERT_NE(orders[i - 1], orders[i]);
        ASSERT_NE(*orders[i - 1], *orders[i]);
    }
}

TEST_F(MemoryPoolTest, MultipleDeallocations)
{
    std::vector<RestingOrder*> orders;

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {
        mp_.deallocate(orders[i - 1]);
    }

    ASSERT_EQ(mp_.get_currently_allocated(), 0);
    ASSERT_EQ(mp_.get_num_slabs(), 1);
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, VerifyMemoryBlockReuse)
{
    std::vector<RestingOrder*> orders;

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {
        mp_.deallocate(orders[i - 1]);
    }

    EXPECT_EQ(mp_.get_currently_allocated(), 0);

    orders.clear();

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {
        ASSERT_EQ(orders[i - 1]->id_, i);
        ASSERT_EQ(orders[i - 1]->quantity_, i);
        ASSERT_EQ(orders[i - 1]->lifetime_, RestingLifetime::GTC);
    }

    ASSERT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    ASSERT_EQ(mp_.get_num_slabs(), 1);
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize);
}

TEST_F(MemoryPoolTest, VerifySlabExpansion)
{
    std::vector<RestingOrder*> orders;

    for (std::uint64_t i = 1; i < initialSlabSize + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize);
    EXPECT_EQ(mp_.get_num_slabs(), 1);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize);

    orders.push_back(mp_.allocate(initialSlabSize + 1, initialSlabSize + 1, RestingLifetime::GTC));

    ASSERT_EQ(mp_.get_currently_allocated(), initialSlabSize + 1);
    ASSERT_EQ(mp_.get_num_slabs(), 2);
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize * 2);

    for (std::uint64_t i = initialSlabSize + 2; i < initialSlabSize * 2 + 1; i++)
    {   
        orders.push_back(mp_.allocate(i, i, RestingLifetime::GTC));
    }

    EXPECT_EQ(mp_.get_currently_allocated(), initialSlabSize * 2);
    EXPECT_EQ(mp_.get_num_slabs(), 2);
    EXPECT_EQ(mp_.get_total_elements(), initialSlabSize * 2);

    orders.push_back(mp_.allocate(initialSlabSize * 2 + 1, initialSlabSize * 2 + 1, RestingLifetime::GTC));

    ASSERT_EQ(mp_.get_currently_allocated(), initialSlabSize * 2 + 1);
    ASSERT_EQ(mp_.get_num_slabs(), 3);
    ASSERT_EQ(mp_.get_total_elements(), initialSlabSize * 4);
}