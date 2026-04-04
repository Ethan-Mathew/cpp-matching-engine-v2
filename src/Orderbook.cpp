#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBook.hpp"
#include "lob/SubmissionResults.hpp"
#include "lob/TimeInForce.hpp"

#include "core/PriceLevel.hpp"
#include "core/RestingOrder.hpp"

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace lob
{

struct OrderBook::Impl
{
    std::unordered_map<OrderID, lob::core::RestingOrder*> idToOrderMap;
    std::map<Price, lob::core::PriceLevel, std::greater<Price>> bidLevels_;
    std::map<Price, lob::core::PriceLevel, std::less<Price>> askLevels_;
};

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

SubmissionResult OrderBook::submit_limit_order(const LimitOrderRequest& limitRequest)
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
    default:
        return SubmissionResult{};
    }

    return SubmissionResult {};
}

template<Side S>
SubmissionResult OrderBook::submit_limit_order_resting(const LimitOrderRequest& limitRequest)
{
    Impl& impl = *pImpl_;
    auto& askLevels = impl.askLevels_;
    auto& bidLevels = impl.bidLevels_;
    auto& idToOrderMap = impl.idToOrderMap;

    Quantity remainingShares = limitRequest.quantity_;
    SubmissionResult subResult {.quantityRequested_ = limitRequest.quantity_};
    
    if constexpr (S == Side::BUY)
    {
        while (remainingShares > 0 && !askLevels.empty())
        {
            auto topOfAsks = askLevels.begin();

            if (crosses<Side::BUY>(limitRequest.price_, topOfAsks->first))
            {
                core::PriceLevel& matchingLevel = topOfAsks->second;

                core::RestingOrder* takingOrder = matchingLevel.front();
                assert(takingOrder != nullptr);

                if (takingOrder->quantity_ > remainingShares)
                {
                    subResult.executions_.emplace_back(takingOrder->id_, topOfAsks->first, remainingShares);
                    matchingLevel.take_shares_from_first(remainingShares);

                    subResult.quantityFilled_ += remainingShares;
                    subResult.status_ = core::SubmitStatus::FILLED;

                    return subResult;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
            
                    subResult.quantityFilled_ += takingOrder->quantity_;

                    subResult.executions_.emplace_back(takingOrder->id_, topOfAsks->first, takingOrder->quantity_);
                    matchingLevel.take_all_shares_from_first();
                    idToOrderMap.erase(takingOrder->id_);
                    
                    //
                    // TODO: deallocate below order with memory pool API
                    // 
                    core::RestingOrder* poppedOrder = matchingLevel.pop_front();

                    if (matchingLevel.empty())
                    {
                        askLevels.erase(topOfAsks);
                    }
                }
            }
            else 
            {
                break;
            }
        }

        if (remainingShares == 0)
        {
            subResult.status_ = core::SubmitStatus::FILLED;
        }
        else if (remainingShares > 0 && remainingShares < limitRequest.quantity_)
        {
            subResult.status_ = core::SubmitStatus::PARTIALLY_FILLED_RESTING;
        }
        else
        {
            subResult.status_ = core::SubmitStatus::RESTING;
        }
    }
    else
    {
        while (remainingShares > 0 && !bidLevels.empty())
        {
            auto topOfBids = bidLevels.begin();

            if (crosses<Side::SELL>(limitRequest.price_, topOfBids->first))
            {
                core::PriceLevel& matchingLevel = topOfBids->second;

                core::RestingOrder* takingOrder = matchingLevel.front();
                assert(takingOrder != nullptr);

                if (takingOrder->quantity_ > remainingShares)
                {
                    subResult.executions_.emplace_back(takingOrder->id_, topOfBids->first, remainingShares);
                    matchingLevel.take_shares_from_first(remainingShares);

                    subResult.quantityFilled_ += remainingShares;
                    subResult.status_ = core::SubmitStatus::FILLED;

                    return subResult;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
            
                    subResult.quantityFilled_ += takingOrder->quantity_;

                    subResult.executions_.emplace_back(takingOrder->id_, topOfBids->first, takingOrder->quantity_);
                    matchingLevel.take_all_shares_from_first();
                    idToOrderMap.erase(takingOrder->id_);
                    
                    //
                    // TODO: deallocate below order with memory pool API
                    // 
                    core::RestingOrder* poppedOrder = matchingLevel.pop_front();

                    if (matchingLevel.empty())
                    {
                        bidLevels.erase(topOfBids);
                    }
                }
            }
            else 
            {
                break;
            }
        }

        if (remainingShares == 0)
        {
            subResult.status_ = core::SubmitStatus::FILLED;
        }
        else if (remainingShares > 0 && remainingShares < limitRequest.quantity_)
        {
            subResult.status_ = core::SubmitStatus::PARTIALLY_FILLED_RESTING;
        }
        else
        {
            subResult.status_ = core::SubmitStatus::RESTING;
        }
    }

    return subResult;
}

void submit_limit_order_fok();
void submit_limit_order_ioc();

} // namespace lob::core