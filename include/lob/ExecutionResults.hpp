#pragma once

#include "lob/OrderBook.hpp"
#include "lob/Aliases.hpp"

#include <cstdint>
#include <vector>

namespace lob
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

} // namespace lob