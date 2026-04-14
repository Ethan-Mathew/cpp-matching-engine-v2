#pragma once

#include <cstdint>

namespace lob::core
{

enum class RestingLifetime : std::uint8_t
{
    GTC,
    DAY
};

} // namespace lob::core