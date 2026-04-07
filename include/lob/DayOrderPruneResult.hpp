#pragma once

#include "Aliases.hpp"

#include <cstddef>

namespace lob
{

struct DayOrderPruneResult
{
    Volume sharesErased             = 0;
    std::uint64_t ordersPruned      = 0;
    std::uint32_t priceLevelsErased = 0;
};

} // namespace lob