#pragma once

#include "lob/Aliases.hpp"

#include "RestingOrder.hpp"

#include <cstddef>

namespace lob::core
{

struct LevelPruneStats
{
    std::size_t ordersPruned_ = 0;
    Quantity quantityPruned_ = 0;
};

} // namespace lob::core