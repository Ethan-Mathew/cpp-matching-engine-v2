#pragma once

#include "lob/Aliases.hpp"

#include <cstdint>

namespace lob
{

struct ExecutionResult
{
    OrderID makerOrderID_;
    Price makerPrice_;
    Quantity executedQuantity_;
};

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