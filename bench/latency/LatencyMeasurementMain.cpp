#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderBookConfig.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include "data/LatencyMeasurementData.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <vector>

using namespace bench::latency;

int main()
{
    std::vector<std::uint64_t> latencies(data::itemsPerMeasurement);
    
    using Clock = std::conditional_t<
        std::chrono::high_resolution_clock::is_steady,
        std::chrono::high_resolution_clock,
        std::chrono::steady_clock
    >;

    const lob::OrderBookConfig obConfig{data::initialPoolSize, data::minPrice, data::maxPrice};
    lob::OrderBook ob{obConfig};

    for (std::size_t i = 0; i < data::itemsPerWarmup; ++i)
    {
        // (OrderID id, Price price, Quantity quantity, Side side, TimeInForce tif)
        lob::LimitOrderRequest limitRequest{
            static_cast<lob::OrderID>(i),
            static_cast<lob::Price>((i % 1000) + 1),
            static_cast<lob::Quantity>(1),
            lob::Side::BUY,
            lob::TimeInForce::GTC
        };

        ob.submit_limit_order(limitRequest);
    }

    for (std::size_t i = 0; i < data::itemsPerMeasurement; ++i)
    {
        lob::LimitOrderRequest limitRequest{
            static_cast<lob::OrderID>(i + data::itemsPerWarmup),
            static_cast<lob::Price>((i % 1000) + 1),
            static_cast<lob::Quantity>(1),
            lob::Side::BUY,
            lob::TimeInForce::GTC
        };

        const auto t1 = Clock::now();
        ob.submit_limit_order(limitRequest);
        const auto t2 = Clock::now();

        const auto deltaT = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        latencies[i] = static_cast<std::uint64_t>(deltaT.count());
    }

    if (ob.get_num_orders() != data::itemsPerWarmup + data::itemsPerMeasurement)
    {
        std::cerr << "Unexpected final order count.\n";
        return EXIT_FAILURE;
    }

    std::sort(latencies.begin(), latencies.end());

    std::ofstream output{"results/latency/resting_limit_submission_latencies.csv"};

    if (!output.is_open())
    {
        std::cerr << "Failed to open latency output file.\n";
        return EXIT_FAILURE;
    }

    output << "latency_ns\n";

    for (auto latency : latencies)
    {
        output << latency << '\n';
    }

    return EXIT_SUCCESS;
}