#pragma once

#include "Aliases.hpp"
#include "ExecutionResults.hpp"

#include <vector>

namespace lob
{

enum class SubmitStatus : std::uint8_t
{
    FILLED,
    PARTIALLY_FILLED_RESTING,
    PARTIALLY_FILLED_CANCELED,
    KILLED,
    REJECTED,
    RESTING,
    CANCELED
};

struct SubmissionResult
{
    Quantity quantityRequested_ = 0;
    Quantity quantityFilled_    = 0;
    SubmitStatus status_;
    std::vector<ExecutionResult> executions_;

    Quantity get_quantity_remaining() const
    {
        return quantityRequested_ - quantityFilled_;
    }
};

} // namespace lob