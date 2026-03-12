#pragma once

#include <cstdint>

using Quantity  = std::int32_t;     // Shares per order
using Price     = std::int64_t;     // Using fixed-point arithmetic - multiplied by 10000
using OrderID   = std::uint64_t;    // Unique order ID
using Volume    = std::uint64_t;    // Shares outstanding