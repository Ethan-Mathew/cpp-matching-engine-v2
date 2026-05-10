#pragma once

#include <cstddef>

enum class OperationKind : std::size_t
{
    LIMIT_SUBMIT = 0,
    MARKET_SUBMIT,
    CANCEL,
    MODIFY,
    SESSION_END,
    COUNT
};

struct OperationWeights
{
    int limitSubmit_ = 0;
    int marketSubmit_ = 0;
    int cancel_ = 0;
    int modify_ = 0;
    int sessionEnd_ = 0;

    int total() const
    {
        return limitSubmit_ + marketSubmit_ + cancel_ + modify_ + sessionEnd_;
    }
};

struct TifWeights
{
    int gtc_ = 0;
    int day_ = 0;
    int ioc_ = 0;
    int fok_ = 0;

    int total() const
    {
        return gtc_ + day_ + ioc_ + fok_;
    }
};