#pragma once

#include "Aliases.hpp"

#include <cstdint>
#include <vector>

namespace lob::core
{

enum class SubmitStatus : std::uint8_t
{
    REJECTED,
    PARTIALLY_FILLED,
    CANCELLED_REMAINDER,
    RESTING,
    FILLED
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

struct ExecutionResult
{
    OrderID touchedOrderID_;
    Price touchedPrice;
    Quantity touchedQuantity;
};

struct SubmissionResult
{
    Quantity quantityFilled_;
    Quantity quantityRemaining_;
    SubmitStatus status_;
    std::vector<ExecutionResult> executions_;
};

} // namespace lob::core