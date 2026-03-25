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

        if constexpr (S == Side::BUY)
        {
            if (!topOfAsks.empty())
            {

            }

            auto topOfAsks = askLevels_.begin();

            std::vector<RestingOrder*> executedOrders;

            while (remainingShares > 0 &&
                   !matchLevel.empty() &&
                   !askLevels_.empty() &&
                   crosses<Side::BUY>(limitRequest.price_, topOfAsks->first))
            {
                // execute on the price level
                core::RestingOrder* takingOrder = matchLevel.front();
                
                // If the first order in the level has enough quantity to fulfill the request
                if (takingOrder->quantity_ >= remainingShares)
                {
                    matchLevel.take_shares(remainingShares);
                    remainingShares = 0;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
                    matchLevel.take_all_shares();
                }
                
                if (takingOrder->empty())
                {
                    executedOrders.push_back(matchLevel->pop_front());
                }

                if (matchLevel.empty())
                {
                    askLevels_.erase(topOfAsks->first);
                    topOfAsks = askLevels_.begin();
                }
            }

            if (remainingShares > 0)
            {
                // rest the remaining if there is remaining
                // need to allocate on memory pool to create new order, then push back on the price level
            }

            // return execution result
            
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