#pragma once

#include "lob/Aliases.hpp"

#include "MixedWorkloadOperations.hpp"

#include <array>
#include <string_view>

static constexpr std::size_t initialSlabSize = 100'000;
static constexpr lob::Price minPrice = 9'990;
static constexpr lob::Price maxPrice = 10'010;
static constexpr lob::Quantity minQty = 1;
static constexpr lob::Quantity maxQty = 20;

static constexpr std::size_t operationKindCount = static_cast<std::size_t>(OperationKind::COUNT);

static constexpr std::array<std::string_view, operationKindCount> operationKindNames{
    "limit_submit",
    "market_submit",
    "cancel",
    "modify",
    "session_end"
};