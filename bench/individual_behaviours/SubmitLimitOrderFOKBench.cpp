#include <benchmark/benchmark.h>

#include <cstddef>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderBookConfig.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

using namespace lob;

// FOK Limit order submissions that are all killed with no pre-scan
static void BM_SubmitFOKNoFill(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            LimitOrderRequest request{i + 1, 1, 1, Side::BUY, TimeInForce::FOK};

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Submit FOK limit orders that pre-scan the entire book and are killed
template <std::size_t ScanDepth> static void BM_SubmitFOKKilledAfterScan(benchmark::State& state) {
    static_assert(ScanDepth > 0);

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize * ScanDepth + 1024, 10000,
                                     static_cast<Price>(10'000 + ScanDepth - 1)}};

        for (std::size_t i = 0; i < ScanDepth; ++i) {
            LimitOrderRequest restingAsk{i + 1, static_cast<Price>(10'000 + i), 1, Side::SELL,
                                         TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            LimitOrderRequest request{ScanDepth + i + 1, static_cast<Price>(10'000 + ScanDepth - 1),
                                      static_cast<Quantity>(ScanDepth + 1), Side::BUY,
                                      TimeInForce::FOK};

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// FOK limit orders that each fill on unique price levels
static void BM_SubmitFOKSameLevelFullFill(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};

        for (std::size_t i = 1; i <= batchSize; ++i) {
            LimitOrderRequest restingAsk{i, 1, 1, Side::SELL, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        const std::size_t doubleBatchSize = batchSize << 1;

        for (std::size_t i = batchSize + 1; i <= doubleBatchSize; ++i) {
            LimitOrderRequest request{i, 1, 1, Side::BUY, TimeInForce::FOK};

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// FOK limit orders that must sweep multiple levels to fill
template <std::size_t SweepDepth> static void BM_SubmitFOKSweepLevels(benchmark::State& state) {
    static_assert(SweepDepth > 0);

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        const std::size_t restingOrderCount = batchSize * SweepDepth;

        OrderBook ob{OrderBookConfig{restingOrderCount + 1024, 10000,
                                     static_cast<Price>(10'000 + restingOrderCount - 1)}};

        for (std::size_t i = 0; i < restingOrderCount; ++i) {
            LimitOrderRequest restingAsk{i + 1, static_cast<Price>(10'000 + i), 1, Side::SELL,
                                         TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            const Price buyPrice = static_cast<Price>(10'000 + ((i + 1) * SweepDepth) - 1);

            LimitOrderRequest request{restingOrderCount + i + 1, buyPrice,
                                      static_cast<Quantity>(SweepDepth), Side::BUY,
                                      TimeInForce::FOK};

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_SubmitFOKNoFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitFOKKilledAfterScan<1>)->Arg(1'000)->Arg(10'000)->Arg(100'000);
BENCHMARK(BM_SubmitFOKKilledAfterScan<5>)->Arg(1'000)->Arg(10'000)->Arg(100'000);
BENCHMARK(BM_SubmitFOKKilledAfterScan<10>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitFOKKilledAfterScan<50>)->Arg(1'000)->Arg(10'000);

BENCHMARK(BM_SubmitFOKSameLevelFullFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitFOKSweepLevels<1>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitFOKSweepLevels<2>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitFOKSweepLevels<5>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitFOKSweepLevels<10>)->Arg(1'000)->Arg(10'000);