#pragma once

#include "lob/OrderBook.hpp"

#include "Aliases.hpp"

#include <cstdint>
#include <vector>

namespace lob::core
{

enum class CancelResult : std::uint8_t
{
    NOT_FOUND,
    CANCELLED
};

enum class ModificationResult : std::uint8_t
{
    NOT_FOUND,
    MODIFIED
};

} // namespace lob::core