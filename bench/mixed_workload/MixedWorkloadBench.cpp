#include "data/MixedWorkloadConfig.hpp"
#include "data/MixedWorkloadData.hpp"
#include "data/MixedWorkloadLoggingHelpers.hpp"
#include "data/MixedWorkloadOperations.hpp"
#include "data/MixedWorkloadResultCounters.hpp"

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <vector>

using namespace lob;

class MixedWorkloadRunner
{
public:
    explicit MixedWorkloadRunner(const WorkloadConfig& config)
        : config_{config}
        , ob_{initialSlabSize}
        , rng_{config.seed_}
    {
        allLatencies_.reserve(config.operationCount_);

        for (auto& samples : latenciesByKind_)
        {
            samples.reserve(config.operationCount_ / operationKindCount);
        }

        seenIds_.reserve(config.operationCount_);
        likelyLiveIds_.reserve(config.operationCount_);
    }

    void run()
    {
        for (std::size_t i = 0; i < config_.operationCount_; ++i)
        {
            run_one_operation();
        }
    }

    void print_report() const
    {
        std::cout << "\n=== Mixed Workload: " << config_.name_ << " ===\n";
        std::cout << "seed: " << config_.seed_ << "\n";
        std::cout << "operations: " << config_.operationCount_ << "\n\n";

        print_summary_row("overall", summarize(allLatencies_));

        std::cout << "\nBy operation type:\n";

        for (std::size_t i = 0; i < operationKindCount; ++i)
        {
            if (!latenciesByKind_[i].empty())
            {
                print_summary_row(operationKindNames[i], summarize(latenciesByKind_[i]));
            }
        }

        print_result_counters();

        std::cout << '\n';
    }

private:
    int random_int(int lo, int hi)
    {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng_);
    }

    Price random_price()
    {
        return static_cast<Price>(random_int(static_cast<int>(minPrice), static_cast<int>(maxPrice)));
    }

    Quantity random_quantity()
    {
        return static_cast<Quantity>(random_int(static_cast<int>(minQty), static_cast<int>(maxQty)));
    }

    Side random_side()
    {
        return random_int(0, 1) == 0 ? Side::BUY : Side::SELL;
    }

    TimeInForce random_tif()
    {
        const int pick = random_int(0, config_.tifWeights_.total() - 1);

        if (pick < config_.tifWeights_.gtc_)
        {
            return TimeInForce::GTC;
        }

        if (pick < config_.tifWeights_.gtc_ + config_.tifWeights_.day_)
        {
            return TimeInForce::DAY;
        }

        if (pick < config_.tifWeights_.gtc_ + config_.tifWeights_.day_ + config_.tifWeights_.ioc_)
        {
            return TimeInForce::IOC;
        }

        return TimeInForce::FOK;
    }

    OperationKind random_operation_kind()
    {
        const int pick = random_int(0, config_.opWeights_.total() - 1);

        if (pick < config_.opWeights_.limitSubmit_)
        {
            return OperationKind::LIMIT_SUBMIT;
        }

        if (pick < config_.opWeights_.limitSubmit_ + config_.opWeights_.marketSubmit_)
        {
            return OperationKind::MARKET_SUBMIT;
        }

        if (pick < config_.opWeights_.limitSubmit_ + config_.opWeights_.marketSubmit_ + config_.opWeights_.cancel_)
        {
            return OperationKind::CANCEL;
        }

        if (pick < config_.opWeights_.limitSubmit_ + config_.opWeights_.marketSubmit_ + config_.opWeights_.cancel_ + config_.opWeights_.modify_)
        {
            return OperationKind::MODIFY;
        }

        return OperationKind::SESSION_END;
    }

    OrderID random_submit_id()
    {
        // Mostly fresh IDs, occasionally reused IDs to exercise duplicate rejection.
        if (!seenIds_.empty() && random_int(0, 99) < 5)
        {
            return seenIds_[static_cast<std::size_t>(
                random_int(0, static_cast<int>(seenIds_.size() - 1)))];
        }

        return nextId_++;
    }

    OrderID random_cancel_or_modify_id()
    {
        // Mostly target IDs that are likely still resting.
        if (!likelyLiveIds_.empty() && random_int(0, 99) < 80)
        {
            return likelyLiveIds_[static_cast<std::size_t>(
                random_int(0, static_cast<int>(likelyLiveIds_.size() - 1)))];
        }

        // Sometimes target previously seen IDs to exercise stale/missing IDs.
        if (!seenIds_.empty() && random_int(0, 99) < 50)
        {
            return seenIds_[static_cast<std::size_t>(
                random_int(0, static_cast<int>(seenIds_.size() - 1)))];
        }

        // Sometimes target definitely invalid IDs.
        return nextId_ + static_cast<OrderID>(random_int(10'000, 20'000));
    }

    void run_one_operation()
    {
        const OperationKind kind = random_operation_kind();

        switch (kind)
        {
        case OperationKind::LIMIT_SUBMIT:
            run_limit_submit();
            break;

        case OperationKind::MARKET_SUBMIT:
            run_market_submit();
            break;

        case OperationKind::CANCEL:
            run_cancel();
            break;

        case OperationKind::MODIFY:
            run_modify();
            break;

        case OperationKind::SESSION_END:
            run_session_end();
            break;

        case OperationKind::COUNT:
            break;
        }
    }

    template <typename Func>
    auto record_latency(OperationKind kind, Func&& func)
    {
        const auto start = Clock::now();
        auto result = func();
        const auto stop = Clock::now();

        const auto ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count()
        );

        allLatencies_.push_back(ns);
        latenciesByKind_[static_cast<std::size_t>(kind)].push_back(ns);

        benchmark_escape(result);

        return result;
    }

    void run_limit_submit()
    {
        const OrderID id = random_submit_id();

        LimitOrderRequest request{
            id,
            random_price(),
            random_quantity(),
            random_side(),
            random_tif()
        };

        SubmissionResult result = record_latency(OperationKind::LIMIT_SUBMIT, [&]
        {
            return ob_.submit_limit_order(request);
        });

        ++resultCounters_.limitSubmitStatuses_[static_cast<std::size_t>(result.status_)];

        seenIds_.push_back(id);

        if (is_resting_submit_result(result))
        {
            add_likely_live_id(id);
        }
    }

    void run_market_submit()
    {
        const OrderID id = random_submit_id();

        MarketOrderRequest request{
            id,
            random_quantity(),
            random_side()
        };

        SubmissionResult result = record_latency(OperationKind::MARKET_SUBMIT, [&]
        {
            return ob_.submit_market_order(request);
        });

        ++resultCounters_.marketSubmitStatuses_[static_cast<std::size_t>(result.status_)];

        seenIds_.push_back(id);

        // Market orders never rest, so do not add this ID to likelyLiveIds_.
    }

    void run_cancel()
    {
        const OrderID id = random_cancel_or_modify_id();

        CancelOrderRequest request{id};

        CancelResult result = record_latency(OperationKind::CANCEL, [&]
        {
            return ob_.cancel_order(request);
        });

        ++resultCounters_.cancelStatuses_[static_cast<std::size_t>(result.status_)];

        if (result.status_ == CancelStatus::CANCELED)
        {
            remove_likely_live_id(id);
        }
    }

    void run_modify()
    {
        const OrderID id = random_cancel_or_modify_id();

        ModifyOrderRequest request{
            id,
            static_cast<Quantity>(random_int(0, static_cast<int>(maxQty))),
            random_price()
        };

        ModificationResult result = record_latency(OperationKind::MODIFY, [&]
        {
            return ob_.modify_order(request);
        });

        ++resultCounters_.modificationStatuses_[static_cast<std::size_t>(result.status_)];

        if (result.status_ == ModificationStatus::NOT_FOUND)
        {
            return;
        }

        if (result.status_ == ModificationStatus::CANCELED)
        {
            remove_likely_live_id(id);
            return;
        }

        if (result.status_ == ModificationStatus::RESUBMITTED)
        {
            if (result.resubmissionResult_.has_value() &&
                is_resting_submit_result(*result.resubmissionResult_))
            {
                add_likely_live_id(id);
            }
            else
            {
                remove_likely_live_id(id);
            }
        }
    }

    void run_session_end()
    {
        record_latency(OperationKind::SESSION_END, [&]
        {
            return ob_.on_session_end();
        });

        // Conservative: DAY orders may have been pruned, and this harness does not
        // track lifetime per candidate ID. This prevents stale live-ID bias.
        likelyLiveIds_.clear();
    }

    static bool is_resting_submit_result(const SubmissionResult& result)
    {
        return result.status_ == SubmitStatus::RESTING ||
               result.status_ == SubmitStatus::PARTIALLY_FILLED_RESTING;
    }

    void add_likely_live_id(OrderID id)
    {
        if (std::find(likelyLiveIds_.begin(), likelyLiveIds_.end(), id) == likelyLiveIds_.end())
        {
            likelyLiveIds_.push_back(id);
        }
    }

    void remove_likely_live_id(OrderID id)
    {
        auto it = std::find(likelyLiveIds_.begin(), likelyLiveIds_.end(), id);

        if (it != likelyLiveIds_.end())
        {
            *it = likelyLiveIds_.back();
            likelyLiveIds_.pop_back();
        }
    }

    void print_result_counters() const
    {
        std::cout << "\nResult counters:\n";

        print_nonzero_counts<SubmitStatus>(
            "limit submit statuses",
            resultCounters_.limitSubmitStatuses_,
            submit_status_name
        );

        print_nonzero_counts<SubmitStatus>(
            "market submit statuses",
            resultCounters_.marketSubmitStatuses_,
            submit_status_name
        );

        print_nonzero_counts<CancelStatus>(
            "cancel statuses",
            resultCounters_.cancelStatuses_,
            cancel_status_name
        );

        print_nonzero_counts<ModificationStatus>(
            "modification statuses",
            resultCounters_.modificationStatuses_,
            modification_status_name
        );
    }

    template <typename Status, std::size_t N, typename NameFn>
    static void print_nonzero_counts(std::string_view label,
                                     const std::array<std::size_t, N>& counts,
                                     NameFn nameFn)
    {
        std::cout << label << ":\n";

        bool printedAny = false;

        for (std::size_t i = 0; i < counts.size(); ++i)
        {
            if (counts[i] > 0)
            {
                const auto status = static_cast<Status>(i);

                std::cout << "  " << nameFn(status)
                          << ": " << counts[i] << '\n';

                printedAny = true;
            }
        }

        if (!printedAny)
        {
            std::cout << "  none\n";
        }
    }

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

    static LatencySummary summarize(const std::vector<std::uint64_t>& samples)
    {
        LatencySummary summary;

        if (samples.empty())
        {
            return summary;
        }

        std::vector<std::uint64_t> sorted = samples;
        std::sort(sorted.begin(), sorted.end());

        const auto percentile = [&](double p) -> double
        {
            const double rawIndex = p * static_cast<double>(sorted.size() - 1);
            const auto index = static_cast<std::size_t>(rawIndex);
            return static_cast<double>(sorted[index]);
        };

        summary.count_ = sorted.size();
        summary.totalNs_ = std::accumulate(sorted.begin(), sorted.end(), std::uint64_t{0});
        summary.avgNs_ = static_cast<double>(summary.totalNs_) / static_cast<double>(summary.count_);
        summary.p50Ns_ = percentile(0.50);
        summary.p90Ns_ = percentile(0.90);
        summary.p99Ns_ = percentile(0.99);
        summary.maxNs_ = static_cast<double>(sorted.back());

        const double seconds = static_cast<double>(summary.totalNs_) / 1'000'000'000.0;
        summary.throughputOpsPerSec_ = seconds > 0.0
            ? static_cast<double>(summary.count_) / seconds
            : 0.0;

        return summary;
    }

    static void print_summary_row(std::string_view label, const LatencySummary& summary)
    {
        std::cout << std::left << std::setw(20) << label
                  << " count=" << std::right << std::setw(10) << summary.count_
                  << " avg_ns=" << std::setw(12) << std::fixed << std::setprecision(2) << summary.avgNs_
                  << " p50_ns=" << std::setw(12) << summary.p50Ns_
                  << " p90_ns=" << std::setw(12) << summary.p90Ns_
                  << " p99_ns=" << std::setw(12) << summary.p99Ns_
                  << " max_ns=" << std::setw(12) << summary.maxNs_
                  << " ops/sec=" << std::setw(14) << summary.throughputOpsPerSec_
                  << '\n';
    }

    template <typename T>
    static void benchmark_escape(const T& value)
    {
#if defined(__GNUC__) || defined(__clang__)
        asm volatile("" : : "g"(&value) : "memory");
#else
        (void)value;
#endif
    }

    using Clock = std::chrono::steady_clock;

    WorkloadConfig config_;
    OrderBook ob_;
    std::mt19937_64 rng_;
    OrderID nextId_ = 1;

    std::vector<OrderID> seenIds_;
    std::vector<OrderID> likelyLiveIds_;

    std::vector<std::uint64_t> allLatencies_;
    std::array<std::vector<std::uint64_t>, operationKindCount> latenciesByKind_;

    ResultCounters resultCounters_;
};

int main()
{
    const std::vector<WorkloadConfig> workloads{
        WorkloadConfig{
            .name_ = "passive_heavy",
            .operationCount_ = 100'000,
            .seed_ = 0xC0FFEE,
            .opWeights_ = OperationWeights{
                .limitSubmit_ = 70,
                .marketSubmit_ = 5,
                .cancel_ = 10,
                .modify_ = 10,
                .sessionEnd_ = 5
            },
            .tifWeights_ = TifWeights{
                .gtc_ = 55,
                .day_ = 25,
                .ioc_ = 10,
                .fok_ = 10
            }
        },
        WorkloadConfig{
            .name_ = "aggressive_heavy",
            .operationCount_ = 100'000,
            .seed_ = 0xA66E5510,
            .opWeights_ = OperationWeights{
                .limitSubmit_ = 40,
                .marketSubmit_ = 30,
                .cancel_ = 10,
                .modify_ = 15,
                .sessionEnd_ = 5
            },
            .tifWeights_ = TifWeights{
                .gtc_ = 30,
                .day_ = 10,
                .ioc_ = 30,
                .fok_ = 30
            }
        },
        WorkloadConfig{
            .name_ = "cancel_modify_heavy",
            .operationCount_ = 100'000,
            .seed_ = 0xBADC0FFEE,
            .opWeights_ = OperationWeights{
                .limitSubmit_ = 40,
                .marketSubmit_ = 5,
                .cancel_ = 25,
                .modify_ = 25,
                .sessionEnd_ = 5
            },
            .tifWeights_ = TifWeights{
                .gtc_ = 60,
                .day_ = 20,
                .ioc_ = 10,
                .fok_ = 10
            }
        },
        WorkloadConfig{
            .name_ = "session_churn",
            .operationCount_ = 100'000,
            .seed_ = 0x51E5510E,
            .opWeights_ = OperationWeights{
                .limitSubmit_ = 60,
                .marketSubmit_ = 5,
                .cancel_ = 10,
                .modify_ = 10,
                .sessionEnd_ = 15
            },
            .tifWeights_ = TifWeights{
                .gtc_ = 25,
                .day_ = 60,
                .ioc_ = 10,
                .fok_ = 5
            }
        }
    };

    for (const WorkloadConfig& config : workloads)
    {
        MixedWorkloadRunner runner{config};
        runner.run();
        runner.print_report();
    }

    return 0;
}