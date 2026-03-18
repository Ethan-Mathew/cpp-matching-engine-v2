#pragma once

#include "Aliases.hpp"
#include "OrderType.hpp"
#include "Side.hpp"
#include "TimeInForce.hpp"

namespace lob
{

struct IncomingOrder
{
    OrderID id_;
    Price price_;
    Quantity quantity_;
    Side side_;
    OrderType type_;
    TimeInForce tif_;
    
    IncomingOrder() = delete;

    IncomingOrder(OrderID id, Price price, Quantity quantity, Side side, OrderType type, TimeInForce tif)
        : id_{id}
        , price_{price}
        , quantity_{quantity}
        , side_{side}
        , type_{type}
        , tif_{tif}
    {
    }
};

} // namespace lob