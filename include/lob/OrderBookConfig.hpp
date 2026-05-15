#pragma once

#include "Aliases.hpp"

#include <cstddef>

namespace lob
{

struct OrderBookConfig
{
    std::size_t initialSlabSize_;
    Price minPrice_;
    Price maxPrice_;
};

} // namespace lob