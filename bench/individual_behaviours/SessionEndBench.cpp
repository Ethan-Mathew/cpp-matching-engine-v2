#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

// Session-end pass over one bid level containing only GTC orders, pruning nothing
static void BM_SessionEndAllGTCSameLevel(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingBid{
                i,
                1,
                1,
                Side::BUY,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Session-end pass over one bid level containing only DAY orders, pruning all orders
static void BM_SessionEndAllDAYSameLevel(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingBid{
                i,
                1,
                1,
                Side::BUY,
                TimeInForce::DAY
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Session-end pass over one bid level with alternating DAY/GTC orders
static void BM_SessionEndMixedSameLevel(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            const TimeInForce tif = (i % 2 == 0) ? TimeInForce::DAY : TimeInForce::GTC;

            LimitOrderRequest restingBid{
                i,
                1,
                1,
                Side::BUY,
                tif
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Session-end pass over many bid levels containing only GTC orders, pruning nothing
static void BM_SessionEndAllGTCUniqueLevels(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingBid{
                i,
                static_cast<Price>(i),
                1,
                Side::BUY,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Session-end pass over many bid levels containing only DAY orders, pruning all orders and levels
static void BM_SessionEndAllDAYUniqueLevels(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingBid{
                i,
                static_cast<Price>(i),
                1,
                Side::BUY,
                TimeInForce::DAY
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Session-end pass over many bid levels with one DAY and one GTC order per level
static void BM_SessionEndMixedUniqueLevels(benchmark::State& state)
{
    const std::size_t levelCount = static_cast<std::size_t>(state.range(0));
    const std::size_t orderCount = levelCount * 2;

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{orderCount + 1024};

        for (std::size_t i = 1; i <= levelCount; ++i)
        {
            LimitOrderRequest gtcBid{
                i,
                static_cast<Price>(i),
                1,
                Side::BUY,
                TimeInForce::GTC
            };

            LimitOrderRequest dayBid{
                levelCount + i,
                static_cast<Price>(i),
                1,
                Side::BUY,
                TimeInForce::DAY
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(gtcBid));
            benchmark::DoNotOptimize(ob.submit_limit_order(dayBid));
        }

        state.ResumeTiming();

        benchmark::DoNotOptimize(ob.on_session_end());
        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * orderCount);
}

BENCHMARK(BM_SessionEndAllGTCSameLevel)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SessionEndAllDAYSameLevel)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SessionEndMixedSameLevel)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SessionEndAllGTCUniqueLevels)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SessionEndAllDAYUniqueLevels)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SessionEndMixedUniqueLevels)->Arg(1'000)->Arg(10'000)->Arg(100'000);