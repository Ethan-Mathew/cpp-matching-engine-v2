#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

// Aggressive BUY partially fills resting SELL
static void BM_SubmitAggressiveSameLevelPartialFill(benchmark::State& state)
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

        ob.submit_limit_order(restingAsk);
        
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; i++)
        {
            LimitOrderRequest aggressiveBuy{
                i + 2, 
                1, 
                1, 
                Side::BUY, 
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(aggressiveBuy));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Aggressive BUY fills orders on same level
static void BM_SubmitAggressiveSameLevelFill(benchmark::State& state)
{

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state)
    {
        //LimitOrderRequest(OrderID id, Price price, Quantity quantity, Side side, TimeInForce tif)
        state.PauseTiming();

        OrderBook ob{batchSize + 1024};

        for (std::size_t i = 1; i <= batchSize; i++)
        {
            LimitOrderRequest restingAsk{
                i, 
                1, 
                1, 
                Side::SELL, 
                TimeInForce::GTC
            };
            
            ob.submit_limit_order(restingAsk);
        }

        state.ResumeTiming();

        const std::size_t doubleBatchSize = batchSize << 1;

        for (std::size_t i = batchSize + 1; i <= doubleBatchSize; i++)
        {
            LimitOrderRequest aggressiveBuy{
                i, 
                1, 
                1, 
                Side::BUY, 
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(aggressiveBuy));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Aggressive BUY eliminates (sweeps) multiple price levels at once
template <std::size_t SweepDepth>
static void BM_SubmitAggressiveSweepLevels(benchmark::State& state)
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
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(aggressiveBuy));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_SubmitAggressiveSameLevelPartialFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitAggressiveSameLevelFill)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitAggressiveSweepLevels<1>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitAggressiveSweepLevels<2>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitAggressiveSweepLevels<5>)->Arg(1'000)->Arg(10'000);
BENCHMARK(BM_SubmitAggressiveSweepLevels<10>)->Arg(1'000)->Arg(10'000);