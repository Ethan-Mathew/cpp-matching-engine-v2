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
    PARTIALLY_FILLED_RESTING,
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
    Quantity quantityRequested_ = 0;
    Quantity quantityFilled_    = 0;
    SubmitStatus status_;
    std::vector<OrderBook::ExecutionResult> executions_;

    Quantity get_quantity_remaining() const
    {
        return quantityRequested_ - quantityFilled_;
    }
};

} // namespace lob::core