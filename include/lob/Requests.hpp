#pragma once

#include "Aliases.hpp"
#include "OrderType.hpp"
#include "Side.hpp"
#include "TimeInForce.hpp"

namespace lob
{

struct LimitOrderRequest
{
    OrderID id_;
    Price price_;
    Quantity quantity_;
    Side side_;
    TimeInForce tif_;
    
    LimitOrderRequest() = delete;

    explicit LimitOrderRequest(OrderID id, Price price, Quantity quantity, Side side, TimeInForce tif)
        : id_{id}
        , price_{price}
        , quantity_{quantity}
        , side_{side}
        , tif_{tif}
    {
    }

    bool empty() const
    {
        return quantity_ == 0;
    }
};

struct MarketOrderRequest
{
    OrderID id_;
    Quantity quantity_;
    Side side_;

    MarketOrderRequest() = delete;

    explicit MarketOrderRequest(OrderID id, Quantity quantity, Side side)
        : id_{id}
        , quantity_{quantity}
        , side_{side}
    {
    }

    bool empty() const
    {
        return quantity_ == 0;
    }
};

struct CancelOrderRequest
{
    OrderID id_;
};

} // namespace lob