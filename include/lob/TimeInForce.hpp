#pragma once

#include <cstdint>

namespace lob
{

enum class TimeInForce : std::uint8_t
{
    DAY,
    IOC,
    FOK,
    GTC
};

} // namespace lob