#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

// Modify requests for missing IDs against an empty book
static void BM_ModifyMissingEmptyBook(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{batchSize + 1024};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i)
        {
            ModifyOrderRequest request{
                i + 1,
                1,
                1
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Modify requests that set quantity to zero, becoming cancellations
static void BM_ModifyToZeroQuantity(benchmark::State& state)
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

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            ModifyOrderRequest request{
                i,
                0,
                1
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Modify same-level resting orders to the same price and quantity, forcing cancel plus resubmit
static void BM_ModifySamePriceSameQuantity(benchmark::State& state)
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

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            ModifyOrderRequest request{
                i,
                1,
                1
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Modify resting bid orders to a new non-crossing price, causing cancel plus rest
static void BM_ModifyToNewNonCrossingPrice(benchmark::State& state)
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

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            ModifyOrderRequest request{
                i,
                1,
                static_cast<Price>(2 + (i % 100))
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Modify resting bid orders to an aggressive price, causing each replacement to fully fill
static void BM_ModifyToAggressiveFullFill(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{(batchSize * 2) + 1024};

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

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingAsk{
                batchSize + i,
                10,
                1,
                Side::SELL,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            ModifyOrderRequest request{
                i,
                1,
                10
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Modify resting bid orders to an aggressive price, partially filling and resting the replacement remainder
static void BM_ModifyToAggressivePartialFillRest(benchmark::State& state)
{
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state)
    {
        state.PauseTiming();

        OrderBook ob{(batchSize * 2) + 1024};

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

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            LimitOrderRequest restingAsk{
                batchSize + i,
                10,
                1,
                Side::SELL,
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(restingAsk));
        }

        state.ResumeTiming();

        for (std::size_t i = 1; i <= batchSize; ++i)
        {
            ModifyOrderRequest request{
                i,
                2,
                10
            };

            benchmark::DoNotOptimize(ob.modify_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_ModifyMissingEmptyBook)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_ModifyToZeroQuantity)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_ModifySamePriceSameQuantity)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_ModifyToNewNonCrossingPrice)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_ModifyToAggressiveFullFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_ModifyToAggressivePartialFillRest)->Arg(1'000)->Arg(10'000)->Arg(100'000);