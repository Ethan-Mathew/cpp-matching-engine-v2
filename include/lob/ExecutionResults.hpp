#pragma once

#include "Aliases.hpp"

#include <cstdint>

namespace lob
{

struct ExecutionResult
{
    OrderID makerOrderID_;
    Price makerPrice_;
    Quantity executedQuantity_;
};

enum class ModificationResult : std::uint8_t
{
    NOT_FOUND,
    MODIFIED
};

} // namespace lob