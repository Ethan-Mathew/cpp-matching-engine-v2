#pragma once

#include "Aliases.hpp"

#include <cstddef>

namespace lob::core
{

struct LevelPruneResult
{
    Volume sharesPruned        = 0;
    std::uint32_t ordersPruned = 0;
};

} // namespace lob::core