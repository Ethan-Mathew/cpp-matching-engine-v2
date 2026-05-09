#include <gtest/gtest.h>

#include "lob/Aliases.hpp"
#include "lob/DayOrderPruneResult.hpp"
#include "lob/OrderBook.hpp"
#include "lob/Requests.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace lob;

constexpr std::size_t initialSlabSize = 100;
constexpr Price minPrice = 9990;
constexpr Price maxPrice = 10010;
constexpr Quantity minQty = 1;
constexpr Quantity maxQty = 20;

struct OperationWeights
{
    int limitSubmit_;
    int marketSubmit_;
    int cancel_;
    int modify_;
    int sessionEnd_;

    int total() const
    {
        return limitSubmit_ + marketSubmit_ + cancel_ + modify_ + sessionEnd_;
    }
};

class OrderBookRandomStressTest : public testing::Test
{
protected:
    OrderBookRandomStressTest()
    {
        reset_harness(defaultSeed);
    }

    void reset_harness(std::uint64_t seed)
    {
        ob_ = std::make_unique<OrderBook>(initialSlabSize);
        rng_.seed(seed);
        currentSeed_ = seed;
        nextId_ = 1;
        seenIds_.clear();
        opLog_.clear();
    }

    int random_int(int lo, int hi)
    {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng_);
    }

    Price random_price()
    {
        return static_cast<Price>(
            random_int(static_cast<int>(minPrice), static_cast<int>(maxPrice)));
    }

    Quantity random_quantity()
    {
        return static_cast<Quantity>(
            random_int(static_cast<int>(minQty), static_cast<int>(maxQty)));
    }

    Side random_side()
    {
        return random_int(0, 1) == 0 ? Side::BUY : Side::SELL;
    }

    TimeInForce random_limit_tif()
    {
        const int x = random_int(0, 3);

        switch (x)
        {
        case 0:
            return TimeInForce::GTC;
        case 1:
            return TimeInForce::DAY;
        case 2:
            return TimeInForce::IOC;
        default:
            return TimeInForce::FOK;
        }
    }

    OrderID random_submit_id()
    {
        // Mostly fresh IDs, but occasionally reuse an old one to stress duplicate rejection.
        if (!seenIds_.empty() && random_int(0, 99) < 10)
        {
            return seenIds_[static_cast<std::size_t>(
                random_int(0, static_cast<int>(seenIds_.size() - 1)))];
        }

        return nextId_++;
    }

    OrderID random_seen_or_invalid_id()
    {
        // Mostly IDs we've seen before, but sometimes definitely-invalid IDs.
        if (!seenIds_.empty() && random_int(0, 99) < 80)
        {
            return seenIds_[static_cast<std::size_t>(
                random_int(0, static_cast<int>(seenIds_.size() - 1)))];
        }

        return nextId_ + static_cast<OrderID>(random_int(1000, 2000));
    }

    static const char* side_to_string(Side side)
    {
        return side == Side::BUY ? "BUY" : "SELL";
    }

    static const char* tif_to_string(TimeInForce tif)
    {
        switch (tif)
        {
        case TimeInForce::DAY:
            return "DAY";
        case TimeInForce::IOC:
            return "IOC";
        case TimeInForce::FOK:
            return "FOK";
        case TimeInForce::GTC:
            return "GTC";
        }

        return "UNKNOWN";
    }

    void submit_random_limit()
    {
        const OrderID id = random_submit_id();
        const Price price = random_price();
        const Quantity qty = random_quantity();
        const Side side = random_side();
        const TimeInForce tif = random_limit_tif();

        LimitOrderRequest request{id, price, qty, side, tif};
        SubmissionResult result = ob_->submit_limit_order(request);

        seenIds_.push_back(id);

        std::ostringstream oss;
        oss << "submit_limit"
            << " id=" << id
            << " price=" << price
            << " qty=" << qty
            << " side=" << side_to_string(side)
            << " tif=" << tif_to_string(tif)
            << " status=" << static_cast<int>(result.status_)
            << " filled=" << result.quantityFilled_
            << " remaining=" << result.get_quantity_remaining()
            << " executions=" << result.executions_.size();

        opLog_.push_back(oss.str());
    }

    void submit_random_market()
    {
        const OrderID id = random_submit_id();
        const Quantity qty = random_quantity();
        const Side side = random_side();

        MarketOrderRequest request{id, qty, side};
        SubmissionResult result = ob_->submit_market_order(request);

        seenIds_.push_back(id);

        std::ostringstream oss;
        oss << "submit_market"
            << " id=" << id
            << " qty=" << qty
            << " side=" << side_to_string(side)
            << " status=" << static_cast<int>(result.status_)
            << " filled=" << result.quantityFilled_
            << " remaining=" << result.get_quantity_remaining()
            << " executions=" << result.executions_.size();

        opLog_.push_back(oss.str());
    }

    void cancel_random_order()
    {
        const OrderID id = random_seen_or_invalid_id();

        CancelOrderRequest request{id};
        CancelResult result = ob_->cancel_order(request);

        std::ostringstream oss;
        oss << "cancel"
            << " id=" << id
            << " status=" << static_cast<int>(result.status_)
            << " quantityCanceled=" << result.quantityCancelled_;

        opLog_.push_back(oss.str());
    }

    void modify_random_order()
    {
        const OrderID id = random_seen_or_invalid_id();
        const Quantity newQty = static_cast<Quantity>(random_int(0, static_cast<int>(maxQty)));
        const Price newPrice = random_price();

        ModifyOrderRequest request{id, newQty, newPrice};
        ModificationResult result = ob_->modify_order(request);

        std::ostringstream oss;
        oss << "modify"
            << " id=" << id
            << " newQty=" << newQty
            << " newPrice=" << newPrice
            << " status=" << static_cast<int>(result.status_);

        if (result.resubmissionResult_.has_value())
        {
            const SubmissionResult& sub = *result.resubmissionResult_;
            oss << " resubmitStatus=" << static_cast<int>(sub.status_)
                << " resubmitFilled=" << sub.quantityFilled_
                << " resubmitRemaining=" << sub.get_quantity_remaining()
                << " resubmitExecutions=" << sub.executions_.size();
        }

        opLog_.push_back(oss.str());
    }

    void session_end()
    {
        DayOrderPruneResult result = ob_->on_session_end();

        std::ostringstream oss;
        oss << "session_end"
            << " ordersPruned=" << result.ordersPruned
            << " sharesErased=" << result.sharesErased
            << " priceLevelsErased=" << result.priceLevelsErased;

        opLog_.push_back(oss.str());
    }

    void run_random_operation(const OperationWeights& weights)
    {
        const int op = random_int(0, weights.total() - 1);

        if (op < weights.limitSubmit_)
        {
            submit_random_limit();
        }
        else if (op < weights.limitSubmit_ + weights.marketSubmit_)
        {
            submit_random_market();
        }
        else if (op < weights.limitSubmit_ + weights.marketSubmit_ + weights.cancel_)
        {
            cancel_random_order();
        }
        else if (op < weights.limitSubmit_ + weights.marketSubmit_ + weights.cancel_ + weights.modify_)
        {
            modify_random_order();
        }
        else
        {
            session_end();
        }
    }

    void run_stress_sequence(std::uint64_t seed,
                             std::size_t numSteps,
                             const OperationWeights& weights)
    {
        reset_harness(seed);

        ob_->assert_valid();

        for (std::size_t step = 0; step < numSteps; ++step)
        {
            run_random_operation(weights);

            SCOPED_TRACE(make_failure_context(step));
            ob_->assert_valid();
        }
    }

    std::string make_failure_context(std::size_t step) const
    {
        std::ostringstream oss;
        oss << "seed=" << currentSeed_ << " step=" << step << "\n";

        const std::size_t start = opLog_.size() > 40 ? opLog_.size() - 40 : 0;
        for (std::size_t i = start; i < opLog_.size(); ++i)
        {
            oss << i << ": " << opLog_[i] << "\n";
        }

        return oss.str();
    }

    static constexpr std::uint64_t defaultSeed = 0xFFFFFFFFFFFFFFFFULL;

    std::unique_ptr<OrderBook> ob_;
    std::mt19937_64 rng_;
    std::uint64_t currentSeed_ = defaultSeed;
    OrderID nextId_ = 1;
    std::vector<OrderID> seenIds_;
    std::vector<std::string> opLog_;
};

TEST_F(OrderBookRandomStressTest, MultipleSeedsPreserveInvariants)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Random invariant stress test requires debug assertions enabled.";
#else
    constexpr std::size_t numSteps = 1000;

    const OperationWeights generalProfile{
        .limitSubmit_ = 40,
        .marketSubmit_ = 15,
        .cancel_ = 20,
        .modify_ = 20,
        .sessionEnd_ = 5
    };

    constexpr std::array<std::uint64_t, 8> seeds{
        1ULL,
        2ULL,
        3ULL,
        0xC0FFEEULL,
        0xDEADBEEFULL,
        0xBADC0FFEEULL,
        0x123456789ABCDEF0ULL,
        0xFFFFFFFFFFFFFFFFULL
    };

    for (std::uint64_t seed : seeds)
    {
        run_stress_sequence(seed, numSteps, generalProfile);
    }
#endif
}

TEST_F(OrderBookRandomStressTest, LongRandomRunPreservesInvariants)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Random invariant stress test requires debug assertions enabled.";
#else
    constexpr std::size_t numSteps = 10000;

    const OperationWeights generalProfile{
        .limitSubmit_ = 40,
        .marketSubmit_ = 15,
        .cancel_ = 20,
        .modify_ = 20,
        .sessionEnd_ = 5
    };

    run_stress_sequence(0xBADC0FFEEULL, numSteps, generalProfile);
#endif
}

TEST_F(OrderBookRandomStressTest, CancelModifyHeavyProfilePreservesInvariants)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Random invariant stress test requires debug assertions enabled.";
#else
    constexpr std::size_t numSteps = 3000;

    const OperationWeights cancelModifyHeavyProfile{
        .limitSubmit_ = 30,
        .marketSubmit_ = 10,
        .cancel_ = 30,
        .modify_ = 25,
        .sessionEnd_ = 5
    };

    run_stress_sequence(0xA11CEULL, numSteps, cancelModifyHeavyProfile);
#endif
}

TEST_F(OrderBookRandomStressTest, SessionEndHeavyProfilePreservesInvariants)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Random invariant stress test requires debug assertions enabled.";
#else
    constexpr std::size_t numSteps = 3000;

    const OperationWeights sessionEndHeavyProfile{
        .limitSubmit_ = 55,
        .marketSubmit_ = 5,
        .cancel_ = 10,
        .modify_ = 10,
        .sessionEnd_ = 20
    };

    run_stress_sequence(0x51E5510EULL, numSteps, sessionEndHeavyProfile);
#endif
}

TEST_F(OrderBookRandomStressTest, AggressiveOrderHeavyProfilePreservesInvariants)
{
#ifdef NDEBUG
    GTEST_SKIP() << "Random invariant stress test requires debug assertions enabled.";
#else
    constexpr std::size_t numSteps = 3000;

    const OperationWeights aggressiveHeavyProfile{
        .limitSubmit_ = 35,
        .marketSubmit_ = 35,
        .cancel_ = 10,
        .modify_ = 15,
        .sessionEnd_ = 5
    };

    run_stress_sequence(0xA66E5510ULL, numSteps, aggressiveHeavyProfile);
#endif
}