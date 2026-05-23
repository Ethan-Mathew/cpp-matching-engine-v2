#include <benchmark/benchmark.h>

#include <cstddef>

#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderBookConfig.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

using namespace lob;

// Cancel requests for missing IDs against an empty book
static void BM_CancelMissingEmptyBook(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};
        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            CancelOrderRequest request{i + 1};

            benchmark::DoNotOptimize(ob.cancel_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Cancel requests for missing IDs against a populated book.
static void BM_CancelMissingPopulatedBook(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};

        for (std::size_t i = 0; i < batchSize; ++i) {
            LimitOrderRequest restingBid{i + 1, 1, 1, Side::BUY, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            CancelOrderRequest request{batchSize + i + 1};

            benchmark::DoNotOptimize(ob.cancel_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Cancel same-level bid orders in FIFO order, repeatedly removing the current
// head
static void BM_CancelSameLevelHead(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};

        for (std::size_t i = 1; i <= batchSize; ++i) {
            LimitOrderRequest restingBid{i, 1, 1, Side::BUY, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        for (std::size_t i = 1; i <= batchSize; ++i) {
            CancelOrderRequest request{i};

            benchmark::DoNotOptimize(ob.cancel_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Cancel same-level bid orders in reverse FIFO order, repeatedly removing the
// current tail
static void BM_CancelSameLevelTail(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, 1}};

        for (std::size_t i = 1; i <= batchSize; ++i) {
            LimitOrderRequest restingBid{i, 1, 1, Side::BUY, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        for (std::size_t i = batchSize; i >= 1; --i) {
            CancelOrderRequest request{i};

            benchmark::DoNotOptimize(ob.cancel_order(request));

            if (i == 1) {
                break;
            }
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Cancel middle orders from independent same-level triples, exercising
// middle-node unlinking
static void BM_CancelSameLevelMiddle(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        const std::size_t restingOrderCount = batchSize * 3;
        OrderBook ob{OrderBookConfig{restingOrderCount + 1024, 1, 1}};

        for (std::size_t i = 1; i <= restingOrderCount; ++i) {
            LimitOrderRequest restingBid{i, 1, 1, Side::BUY, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        for (std::size_t i = 0; i < batchSize; ++i) {
            const OrderID middleId = (i * 3) + 2;
            CancelOrderRequest request{middleId};

            benchmark::DoNotOptimize(ob.cancel_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

// Cancel orders that each occupy a unique bid price level, forcing level
// erasure
static void BM_CancelUniqueLevels(benchmark::State& state) {
    const std::size_t batchSize = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook ob{OrderBookConfig{batchSize + 1024, 1, static_cast<Price>(batchSize)}};

        for (std::size_t i = 1; i <= batchSize; ++i) {
            LimitOrderRequest restingBid{i, static_cast<Price>(i), 1, Side::BUY, TimeInForce::GTC};

            benchmark::DoNotOptimize(ob.submit_limit_order(restingBid));
        }

        state.ResumeTiming();

        for (std::size_t i = 1; i <= batchSize; ++i) {
            CancelOrderRequest request{i};

            benchmark::DoNotOptimize(ob.cancel_order(request));
        }

        benchmark::DoNotOptimize(ob);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
}

BENCHMARK(BM_CancelMissingEmptyBook)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_CancelMissingPopulatedBook)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_CancelSameLevelHead)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_CancelSameLevelTail)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_CancelSameLevelMiddle)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_CancelUniqueLevels)->Arg(1'000)->Arg(10'000)->Arg(100'000);