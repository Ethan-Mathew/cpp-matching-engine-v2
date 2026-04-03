#include <gtest/gtest.h>

#include "MemoryPool.hpp"
#include "RestingLifetime.hpp"
#include "RestingOrder.hpp"

using namespace lob::core;

class MemoryPoolTest : public testing::Test
{
protected:
    MemoryPoolTest()
        : mp_{10}
    {
    }

    MemoryPool mp_;
};