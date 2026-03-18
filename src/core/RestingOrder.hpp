#pragma once

#include "Aliases.hpp"
#include "OrderType.hpp"
#include "PriceLevel.hpp"
#include "RestingLifetime.hpp"
#include "Side.hpp"

struct RestingOrder
{
    RestingOrder* next_ = nullptr;
    RestingOrder* prev_ = nullptr;
    PriceLevel* level_  = nullptr;

    OrderID id_;
    Quantity quantity_;
    RestingLifetime lifetime_;

    RestingOrder() = delete;

    RestingOrder(OrderID id, Quantity quantity, RestingLifetime lifetime)
        : id_{id}
        , quantity_{quantity}
        , lifetime_{lifetime}
    {
    }
};