#pragma once

#include "Aliases.hpp"
#include "PriceLevel.hpp"
#include "RestingOrder.hpp"

#include <map>
#include <unordered_map>

namespace lob
{
    
class OrderBook
{
public:
    
private:
    std::map<Price, PriceLevel*, std::greater<>> bids_;
    std::map<Price, PriceLevel*, std::less<>> asks_;
    std::unordered_map<OrderID, RestingOrder*> allOrders_;
};

};