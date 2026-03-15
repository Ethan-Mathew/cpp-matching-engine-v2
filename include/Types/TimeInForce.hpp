#pragma once

#include <cstdint>

enum class TimeInForce : std::uint8_t
{
    DAY,
    IOC,
    FOK,
    GTC,
    MARKET
};