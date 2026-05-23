#pragma once

#include <cstdint>

namespace lob::replay::data {

using StockLocate = std::uint16_t;
using TrackingNumber = std::uint16_t;
using Timestamp = std::uint64_t;
using OrderReferenceNumber = std::uint64_t;
using MatchNumber = std::uint64_t;
using Shares = std::uint32_t;
using Price = std::uint32_t;

} // namespace lob::replay::data