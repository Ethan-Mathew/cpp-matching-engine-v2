#include "lob/Aliases.hpp"
#include "lob/OrderBook.hpp"
#include "lob/TimeInForce.hpp"

#include "core/PriceLevel.hpp"
#include "core/RestingOrder.hpp"
#include "core/InternalExecutionResults.hpp"

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
    Quantity remainingShares = limitRequest.quantity_;
    std::vector<core::ExecutionResult> executedOrders;
    
    if constexpr (S == Side::BUY)
    {
        while (remainingShares > 0 && !askLevels_.empty())
        {
            auto topOfAsks = askLevels_.begin();

            if (crosses<Side::BUY>(limitRequest.price_, topOfAsks->first))
            {
                core::PriceLevel& matchingLevel = topOfAsks->second;
                core::RestingOrder* takingOrder = matchingLevel.front();

                if (takingOrder->quantity_ > remainingShares)
                {
                    executedOrders.emplace_back(takingOrder->id_, topOfAsks->first, remainingShares);
                    matchingLevel.take_shares_from_first(remainingShares);
                    remainingShares = 0;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
                    executedOrders.emplace_back(takingOrder->id_, topOfAsks->first, takingOrder->quantity_);
                    matchingLevel.take_all_shares_from_first();
                    idToOrderMap.erase(takingOrder->id_);

                    // deallocate below order with memory pool API
                    matchingLevel.pop_front();

                    if (matchingLevel.empty())
                    {
                        askLevels_.erase(topOfAsks);
                    }
                }
            }
            else 
            {
                break;
            }
        }

        if (remainingShares > 0)
        {
            // rest the remaining
            // need to create a new price level
        }
    }
}

void submit_limit_order_fok();
void submit_limit_order_ioc();

} // namespace lob::core