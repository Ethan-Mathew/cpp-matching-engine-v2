    #include "lob/Aliases.hpp"
    #include "lob/OrderBook.hpp"
    #include "lob/TimeInForce.hpp"

    #include "core/PriceLevel.hpp"
    #include "core/RestingOrder.hpp"

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
        std::vector<OrderID> executedOrders;
        
        if constexpr (S == Side::BUY)
        {
            if (!askLevels_.empty())
            {
                while (!askLevels_.empty() &&
                       remainingShares > 0 &&
                       crosses<Side::BUY>(limitRequest.price_, topOfAsks->first))
                {
                    auto topOfAsks = askLevels_.begin();
                    core::PriceLevel& matchLevel = topOfAsks.second;
                    core::RestingOrder* takingOrder = matchLevel.front();
                    
                    // If the first order in the level has enough quantity to fulfill the request
                    if (takingOrder->quantity_ > remainingShares)
                    {
                        matchLevel.take_shares_from_first(remainingShares);
                        remainingShares = 0;
                    }
                    else
                    {
                        remainingShares -= takingOrder->quantity_;
                        matchLevel.take_all_shares_from_first();
                        executedOrders.push_back(takingOrder->id_);
                        //deallocate takingOrder
                    }

                    if (matchLevel.empty())
                    {
                        askLevels_.erase(topOfAsks); // amortized O(1)
                    }
                }
                
                // rest the remaining if there is remaining
                // occurs when asks is empty or limit price doesn't cross
                if (remainingShares > 0)
                {
                    
                    // need to allocate on memory pool to create new order, then push back on the price level
                }

                // return execution result
            }


            
            askLevels_.emplace(limitRequest.price_, core::PriceLevel{limitRequest.price_});

            // allocate new resting order from mem pool and push it onto asklevels
            askLevels_.begin()->second.push_back();
            
            // return result (all shares rested)

        }
    else
    {
        
    }
}

void submit_limit_order_fok();
void submit_limit_order_ioc();

} // namespace lob::core