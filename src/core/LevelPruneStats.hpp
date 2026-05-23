#pragma once

#include <cstddef>

#include "RestingOrder.hpp"
#include "lob/Aliases.hpp"

namespace lob::core {

struct LevelPruneStats {
    std::size_t ordersPruned_ = 0;
    Quantity quantityPruned_ = 0;
};

} // namespace lob::core