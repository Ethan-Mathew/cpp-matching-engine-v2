#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

// All incoming limit orders are cancelled (no volume to take)
static void BM_SubmitIOCNoFill(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{batchSize + 1024};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i)
        {
            LimitOrderRequest request{
                i + 1,
                1,
                1,
                Side::BUY,
                TimeInForce::IOC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Partially filled then cancelled orders
static void BM_SubmitIOCSameLevelPartialFill(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        LimitOrderRequest restingAsk{
            1,
            1,
            static_cast<Quantity>(batchSize + 1),
            Side::SELL,
            TimeInForce::GTC
        };

        benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i)
        {
            LimitOrderRequest request{
                i + 2,
                1,
                1,
                Side::BUY,
                TimeInForce::IOC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Limit orders fully fill against existing liquidity
static void BM_SubmitIOCSameLevelFullFill(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingAsk{
                i,
                1,
                1,
                Side::SELL,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        const std::size_t doubleBatchSize = batchSize << 1;

        for (std::size_t i = batchSize + 1; i <= doubleBatchSize; ++i)
        {
            LimitOrderRequest request{
                i,
                1,
                1,
                Side::BUY,
                TimeInForce::IOC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Limit orders consume multiple levels
template <std::size_t SweepDepth>
static void BM_SubmitIOCSweepLevels(benchmark::State& state)
{
    static_assert(SweepDepth > 0);

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        const std::size_t restingOrderCount = batchSize * SweepDepth;
        OrderBook ob{restingOrderCount + 1024};

        for (std::size_t i = 0; i < restingOrderCount; ++i)
        {
            LimitOrderRequest restingAsk{
                i + 1,
                static_cast<Price>(10'000 + i),
                1,
                Side::SELL,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i)
        {
            const Price buyPrice =
                static_cast<Price>(10'000 + ((i + 1) * SweepDepth) - 1);

            LimitOrderRequest aggressiveBuy{
                restingOrderCount + i + 1,
                buyPrice,
                static_cast<Quantity>(SweepDepth),
                Side::BUY,
                TimeInForce::IOC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(aggressiveBuy));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_SubmitIOCNoFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitIOCSameLevelPartialFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitIOCSameLevelFullFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitIOCSweepLevels<1>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitIOCSweepLevels<2>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitIOCSweepLevels<5>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitIOCSweepLevels<10>)->Arg(1'000)->Arg(10'000);