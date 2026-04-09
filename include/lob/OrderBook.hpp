#pragma once

#include "Aliases.hpp"
#include "DayOrderPruneResult.hpp"
#include "OrderRequests.hpp"
#include "SubmissionResults.hpp"

#include <memory>

namespace lob
{

class OrderBook
{
public:

    OrderBook(std::size_t poolSize);

    SubmissionResult submit_limit_order(const LimitOrderRequest& limitRequest);
    void submit_market_order();
    void modify_order();
    void cancel_order();

    DayOrderPruneResult session_end();

private:
    template<Side S>
    bool crosses(Price orderPrice, Price levelPrice) const;

    void retire_order(core::RestingOrder* order);

    template<typename LevelMap>
    void prune_from_side_map(LevelMap& levelMap, DayOrderPruneResult& dayResult);

    void submit_limit_order_fok();
    void submit_limit_order_ioc();

    template<Side S>
    SubmissionResult submit_limit_order_resting(const LimitOrderRequest& limitRequest);
    
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace lob::core
