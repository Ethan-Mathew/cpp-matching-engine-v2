#pragma once

#include "lob/Aliases.hpp"
#include "lob/Side.hpp"

#include "RestingLifetime.hpp"

namespace lob::core
{

class PriceLevel;

struct RestingOrder
{
    RestingOrder* next_ = nullptr;
    RestingOrder* prev_ = nullptr;
    PriceLevel* level_  = nullptr;

    OrderID id_;
    Quantity quantity_;
    RestingLifetime lifetime_;
    Side side_;

    RestingOrder() = delete;

    RestingOrder(OrderID id, Quantity quantity, RestingLifetime lifetime, Side side)
        : id_{id}
        , quantity_{quantity}
        , lifetime_{lifetime}
        , side_{side}
    {
    }

    RestingOrder(const RestingOrder&) = delete;
    RestingOrder& operator=(const RestingOrder&) = delete;

    bool empty() const
    {
        return quantity_ == 0;
    }

    bool operator==(const RestingOrder& compOrder) const
    {
        return id_ == compOrder.id_;
    }
};

} // namespace lob::core
