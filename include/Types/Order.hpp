#pragma once

#include "Aliases.hpp"
#include "Side.hpp"
#include "TimeInForce.hpp"

#include <cassert>

struct Order
{
    Order* next_ = nullptr;
    Order* prev_ = nullptr;
    // PriceLevel* level_ = nullptr;
    OrderID id_;
    Price price_;
    Quantity quantity_;
    TimeInForce tif_;
    Side side_;

    Order() = delete;

    Order(OrderID id, Price price, Quantity quantity, TimeInForce tif, Side side)
        : id_{id}
        , price_{price}
        , quantity_{quantity}
        , tif_{tif}
        , side_{side}
    {
    }
};