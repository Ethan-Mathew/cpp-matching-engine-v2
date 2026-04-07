#pragma once

#include <cstdint>

namespace lob
{

using Quantity  = std::int32_t;
using Price     = std::int64_t; // Using fixed-point arithmetic - multiplied by 10000
using OrderID   = std::uint64_t;
using Volume    = std::uint64_t;

} // namespace lob