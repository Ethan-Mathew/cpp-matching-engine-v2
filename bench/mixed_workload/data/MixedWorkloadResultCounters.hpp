#pragma once

#include "lob/Results.hpp"

#include <array>
#include <cstddef>

struct ResultCounters
{
    std::array<std::size_t, static_cast<std::size_t>(lob::SubmitStatus::COUNT)> limitSubmitStatuses_{};
    std::array<std::size_t, static_cast<std::size_t>(lob::SubmitStatus::COUNT)> marketSubmitStatuses_{};
    std::array<std::size_t, static_cast<std::size_t>(lob::CancelStatus::COUNT)> cancelStatuses_{};
    std::array<std::size_t, static_cast<std::size_t>(lob::ModificationStatus::COUNT)> modificationStatuses_{};
};