#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

// Market orders that cancel immediately against an empty opposite book
static void BM_SubmitMarketNoFill(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{batchSize + 1024};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i)
        {
            MarketOrderRequest request{
                i + 1,
                1,
                Side::BUY
            };

            benchmark::DoNotOptimize(ob.submit_market_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Market buy orders that each fully consume one maker at one ask level
static void BM_SubmitMarketSameLevelFullFill(benchmark::State& state)
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
            MarketOrderRequest request{
                i,
                1,
                Side::BUY
            };

            benchmark::DoNotOptimize(ob.submit_market_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Market buy orders that each fully sweep a fixed number of ask price levels
template <std::size_t SweepDepth>
static void BM_SubmitMarketSweepLevels(benchmark::State& state)
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
            MarketOrderRequest request{
                restingOrderCount + i + 1,
                static_cast<Quantity>(SweepDepth),
                Side::BUY
            };

            benchmark::DoNotOptimize(ob.submit_market_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_SubmitMarketNoFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitMarketSameLevelFullFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitMarketSweepLevels<1>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitMarketSweepLevels<2>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitMarketSweepLevels<5>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitMarketSweepLevels<10>)->Arg(1'000)->Arg(10'000);