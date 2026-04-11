#pragma once

#include "lob/Aliases.hpp"

#include "RestingOrder.hpp"

#include <vector>

namespace lob::core
{

struct LevelPruneResult
{
    Volume sharesErased = 0;
    std::vector<RestingOrder*> ordersPruned;
};

} // namespace lob::core