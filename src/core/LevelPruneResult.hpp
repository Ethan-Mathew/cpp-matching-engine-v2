#pragma once

#include "Aliases.hpp"
#include "RestingOrder.hpp"

#include <cstddef>
#include <vector>

namespace lob::core
{

struct LevelPruneResult
{
    Volume sharesPrunedCount        = 0;
    std::uint32_t ordersPrunedCount = 0;
    std::vector<RestingOrder*> ordersPruned;
};

} // namespace lob::core