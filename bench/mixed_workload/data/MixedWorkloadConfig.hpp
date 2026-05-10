#pragma once

#include "MixedWorkloadOperations.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

struct WorkloadConfig
{
    std::string name_;
    std::size_t operationCount_ = 0;
    std::uint64_t seed_ = 0;
    OperationWeights opWeights_;
    TifWeights tifWeights_;
};

struct LatencySummary
{
    std::size_t count_ = 0;
    std::uint64_t totalNs_ = 0;
    double avgNs_ = 0.0;
    double p50Ns_ = 0.0;
    double p90Ns_ = 0.0;
    double p99Ns_ = 0.0;
    double maxNs_ = 0.0;
    double throughputOpsPerSec_ = 0.0;
};