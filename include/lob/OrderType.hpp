#pragma once

#include <cstdint>

namespace lob
{

enum class OrderType : std::uint8_t
{
    MARKET,
    LIMIT
};

} // namespace lob