#pragma once

#include <optional>
#include <vector>

#include "Aliases.hpp"
#include "ExecutionResults.hpp"

namespace lob {

enum class SubmitStatus : std::uint8_t {
    FILLED,
    PARTIALLY_FILLED_RESTING,
    PARTIALLY_FILLED_CANCELED,
    KILLED,
    REJECTED,
    RESTING,
    CANCELED,
    COUNT
};

struct SubmissionResult {
    Quantity quantityRequested_ = 0;
    Quantity quantityFilled_ = 0;
    SubmitStatus status_;
    std::vector<ExecutionResult> executions_;

    Quantity get_quantity_remaining() const { return quantityRequested_ - quantityFilled_; }
};

enum class CancelStatus : std::uint8_t { CANCELED, NOT_FOUND, COUNT };

struct CancelResult {
    Quantity quantityCancelled_ = 0;
    CancelStatus status_;
};

enum class ModificationStatus : std::uint8_t { RESUBMITTED, CANCELED, REJECTED, NOT_FOUND, COUNT };

struct ModificationResult {
    Quantity originalQuantity_ = 0;
    ModificationStatus status_;
    std::optional<SubmissionResult> resubmissionResult_;
};

struct DayOrderPruneResult {
    Volume sharesErased = 0;
    std::uint64_t ordersPruned = 0;
    std::uint32_t priceLevelsErased = 0;
};

} // namespace lob