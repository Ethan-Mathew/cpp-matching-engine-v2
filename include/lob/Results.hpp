#pragma once

#include "Aliases.hpp"
#include "ExecutionResults.hpp"

#include <optional>
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

enum class CancelStatus : std::uint8_t
{
    CANCELED,
    NOT_FOUND
};

struct CancelResult
{
    Quantity quantityCancelled_ = 0;
    CancelStatus status_;
};

enum class ModificationStatus : std::uint8_t
{
    RESUBMITTED,
    CANCELED,
    NOT_FOUND
};

struct ModificationResult
{
    Quantity originalQuantity_ = 0;
    ModificationStatus status_;
    std::optional<SubmissionResult> resubmissionResult_;
};

} // namespace lob