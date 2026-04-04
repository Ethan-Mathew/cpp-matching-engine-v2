#pragma once

#include "Aliases.hpp"
#include "ExecutionResults.hpp"
#include "OrderRequests.hpp"
#include "PriceLevel.hpp"
#include "RestingOrder.hpp"
#include "SubmissionResults.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace lob
{

class OrderBook
{
public:

    struct ExecutionResult
    {
        OrderID makerOrderID_;
        Price makerPrice;
        Quantity executedQuantity;
    };

    OrderBook() = default;

    SubmissionResult submit_limit_order(const LimitOrderRequest& limitRequest);
    void submit_market_order();
    void modify_order();
    void cancel_order();
    void prune_good_for_day_orders();

private:
    template<Side S>
    bool crosses(Price orderPrice, Price levelPrice) const;

    void submit_limit_order_fok();
    void submit_limit_order_ioc();

    template<Side S>
    SubmissionResult submit_limit_order_resting(const LimitOrderRequest& limitRequest);
    
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace lob::core
