#pragma once

#include "Aliases.hpp"
#include "OrderBook.hpp"

#include <cstdint>
#include <vector>

namespace lob::core
{

enum class SubmitStatus : std::uint8_t
{
    FILLED,
    PARTIALLY_FILLED,
    REJECTED,
    RESTING,
    CANCELED
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

struct SubmissionResult
{
    Quantity quantityFilled_;
    Quantity quantityRemaining_;
    SubmitStatus status_;
    std::vector<OrderBook::ExecutionResult> executions_;
};

} // namespace lob::core