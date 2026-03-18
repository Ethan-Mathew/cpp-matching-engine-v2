#pragma once

#include "lob/Aliases.hpp"
#include "lob/OrderType.hpp"
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

    lob::OrderID id_;
    lob::Quantity quantity_;
    RestingLifetime lifetime_;

    RestingOrder() = delete;

    RestingOrder(lob::OrderID id, lob::Quantity quantity, RestingLifetime lifetime)
        : id_{id}
        , quantity_{quantity}
        , lifetime_{lifetime}
    {
    }

    RestingOrder(const RestingOrder&) = delete;
    RestingOrder& operator=(const RestingOrder&) = delete;
};

} // namespace lob::core
