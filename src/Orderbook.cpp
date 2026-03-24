#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/TimeInForce.hpp"

#include "PriceLevel.hpp"
#include "RestingOrder.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace lob
{

/*
    OrderID id_;
    Price price_;
    Quantity quantity_;
    Side side_;
    TimeInForce tif_;

*/

template<Side S>
bool crosses(Price orderPrice, Price levelPrice)
{
    if constexpr (S == Side::BUY)
    {
        return orderPrice >= levelPrice;
    }
    else
    {
        return orderPrice <= levelPrice;
    }
}

void OrderBook::submit_limit_order(const LimitOrderRequest& limitRequest)
{
    switch(limitRequest.tif_)
    {
    case TimeInForce::GTC:
    case TimeInForce::DAY:
        if (limitRequest.side_ == Side::BUY)
        {
            return submit_limit_order_resting<Side::BUY>(limitRequest);
        }
        else 
        {
            return submit_limit_order_resting<Side::SELL>(limitRequest);
        }
    case TimeInForce::IOC:
    case TimeInForce::FOK:
    }
}

template<Side S>
void OrderBook::submit_limit_order_resting(const LimitOrderRequest& limitRequest)
{
    if constexpr (S == Side::BUY)
    {
        auto topOfAsks = askLevels_.begin();
        auto levelIt = topOfAsks->second;
        std::vector<OrderID> executed;

        while (!limitRequest.empty() &&
               !levelIt->empty() &&
               crosses<Side::BUY>(limitRequest.price_, topOfAsks->first))
        {
            // execute on the price level
            RestingOrder* takingOrder = levelIt->front();
            
            // If the first order in the level has enough quantity to fulfill the request
            if (takingOrder->quantity_ >= limitRequest.quantity_)
            {
                takingOrder->quantity_ -= limitRequest.quantity_;
                limitRequest.quantity_ = 0;
            }
            else
            {
                limitRequest.quantity_ -= takingOrder->quantity_;
                takingOrder->quantity_ = 0;
            }
            
            if (takingOrder.empty())
            {
                executed.push_back(levelIt->pop_front());
            }
        }

        if (!limitRequest.empty())
        {
            // rest the remaining if there is remaining
            // need to allocate on memory pool to create new order, then push back on the price level
        }

        // return execution result
        
    }
    else
    {
        
    }
}

void submit_limit_order_fok();
void submit_limit_order_ioc();

} // namespace lob::core