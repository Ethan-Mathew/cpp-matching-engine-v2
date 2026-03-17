#pragma once

#include "Aliases.hpp"
#include "OrderType.hpp"
#include "PriceLevel.hpp"
#include "RestingOrderPolicy.hpp"
#include "Side.hpp"

struct RestingOrder
{
    RestingOrder* next_ = nullptr;
    RestingOrder* prev_ = nullptr;
    PriceLevel* level_ = nullptr;

    OrderID id_;
    Quantity quantity_;
    RestingOrderPolicy policy_;

    RestingOrder() = delete;

    RestingOrder(OrderID id, Quantity quantity, RestingOrderPolicy policy)
        : id_{id}
        , quantity_{quantity}
        , policy_{policy}
    {
    }
};