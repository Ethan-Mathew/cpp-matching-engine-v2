#pragma once

#include "lob/Aliases.hpp"

#include <cstddef>

namespace bench::latency::data
{

static constexpr std::size_t itemsPerWarmup = 250'000;
static constexpr std::size_t itemsPerMeasurement = 10'000'000;
static constexpr std::size_t initialPoolSize = 10'250'001;
static constexpr lob::Price minPrice = 1;
static constexpr lob::Price maxPrice = 1000;

} // namespace bench::latency::data