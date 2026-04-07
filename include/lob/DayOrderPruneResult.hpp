#pragma once

#include "Aliases.hpp"

#include <cstddef>

namespace lob
{

struct DayOrderPruneResult
{
    Volume sharesEliminated             = 0;
    std::uint64_t ordersPruned          = 0;
    std::uint32_t priceLevelsEliminated = 0;
};

} // namespace lob