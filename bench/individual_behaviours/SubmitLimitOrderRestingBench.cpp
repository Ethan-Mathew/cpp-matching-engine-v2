#include <benchmark/benchmark.h>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderBookConfig.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <cstddef>

using namespace lob;

static void BM_SubmitRestingSameLevel(benchmark::State& state)
{

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; i++)
        {
            LimitOrderRequest limitRequest{
                i + 1, 
                1, 
                1, 
                Side::BUY, 
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(limitRequest));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

static void BM_SubmitRestingDiffLevels(benchmark::State& state)
{

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, static_cast<Price>(batchSize)}};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; i++)
        {
            LimitOrderRequest limitRequest{
                i + 1, 
                static_cast<Price>(i + 1), 
                1, 
                Side::BUY, 
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(limitRequest));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

template <std::size_t Spread>
static void BM_SubmitRestingDiffLevelsFixedSpread(benchmark::State& state)
{

    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));
    
    for (auto _ : state)
    {
        state.PauseTiming();
        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, static_cast<Price>(Spread)}};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; i++)
        {
            LimitOrderRequest limitRequest{
                i + 1, 
                static_cast<Price>((i % Spread) + 1), 
                1, 
                Side::BUY, 
                TimeInForce::GTC
            };

            benchmark::DoNotOptimize(ob.submit_limit_order(limitRequest));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_SubmitRestingSameLevel)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitRestingDiffLevels)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_SubmitRestingDiffLevelsFixedSpread<10>)->Arg(1'000)->Arg(10'000)->Arg(100'000);
BENCHMARK(BM_SubmitRestingDiffLevelsFixedSpread<100>)->Arg(1'000)->Arg(10'000)->Arg(100'000);
BENCHMARK(BM_SubmitRestingDiffLevelsFixedSpread<1'000>)->Arg(1'000)->Arg(10'000)->Arg(100'000);