#pragma once

#include "lob/Results.hpp"

#include <string_view>

using namespace lob;

static std::string_view submit_status_name(SubmitStatus status)
{
    switch (status)
    {
    case SubmitStatus::RESTING:
        return "RESTING";
    case SubmitStatus::FILLED:
        return "FILLED";
    case SubmitStatus::PARTIALLY_FILLED_RESTING:
        return "PARTIALLY_FILLED_RESTING";
    case SubmitStatus::PARTIALLY_FILLED_CANCELED:
        return "PARTIALLY_FILLED_CANCELED";
    case SubmitStatus::CANCELED:
        return "CANCELED";
    case SubmitStatus::KILLED:
        return "KILLED";
    case SubmitStatus::REJECTED:
        return "REJECTED";
    case SubmitStatus::COUNT:
        return "COUNT";
    }

    return "UNKNOWN";
}

static std::string_view cancel_status_name(CancelStatus status)
{
    switch (status)
    {
    case CancelStatus::CANCELED:
        return "CANCELED";
    case CancelStatus::NOT_FOUND:
        return "NOT_FOUND";
    case CancelStatus::COUNT:
        return "COUNT";
    }

    return "UNKNOWN";
}

static std::string_view modification_status_name(ModificationStatus status)
{
    switch (status)
    {
    case ModificationStatus::RESUBMITTED:
        return "RESUBMITTED";
    case ModificationStatus::CANCELED:
        return "CANCELED";
    case ModificationStatus::NOT_FOUND:
        return "NOT_FOUND";
    case ModificationStatus::COUNT:
        return "COUNT";
    }

    return "UNKNOWN";
}