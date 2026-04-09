#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBook.hpp"
#include "lob/OrderRequests.hpp"
#include "lob/OrderType.hpp"
#include "lob/Side.hpp"
#include "lob/SubmissionResults.hpp"
#include "lob/TimeInForce.hpp"

#include "core/LevelPruneResult.hpp"
#include "core/MemoryPool.hpp"
#include "core/PriceLevel.hpp"
#include "core/RestingLifetime.hpp"
#include "core/RestingOrder.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lob
{

OrderBook::OrderBook(std::size_t poolSize)
    : pImpl_{std::make_unique<Impl>(poolSize)}
{    
}

struct OrderBook::Impl
{
    core::MemoryPool memoryPool_;
    std::unordered_map<OrderID, core::RestingOrder*> idToOrderMap_;
    std::map<Price, core::PriceLevel, std::greater<Price>> bidLevels_;
    std::map<Price, core::PriceLevel, std::less<Price>> askLevels_;

    Impl(std::size_t poolSize)
        : memoryPool_{poolSize}
    {
    }
};

OrderBook::~OrderBook() = default;

std::size_t OrderBook::get_num_orders() const
{
    return pImpl_->idToOrderMap_.size();
}

std::size_t OrderBook::get_num_levels_bids() const
{
    return pImpl_->bidLevels_.size();
}

std::size_t OrderBook::get_num_levels_asks() const
{
    return pImpl_->askLevels_.size();
}

std::size_t OrderBook::get_memory_pool_size() const
{
    return pImpl_->memoryPool_.get_total_elements();
}

template<Side S>
bool OrderBook::crosses(Price orderPrice, Price levelPrice) const
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

template <class RestingOrderType>
void OrderBook::retire_order(RestingOrderType* order)
{
    pImpl_->idToOrderMap_.erase(order->id_);
    pImpl_->memoryPool_.deallocate(order);
}

SubmissionResult OrderBook::submit_limit_order(const LimitOrderRequest& limitRequest)
{
    auto& idToOrderMap = pImpl_->idToOrderMap_;
    
    if (idToOrderMap.contains(limitRequest.id_))
    {
        return SubmissionResult{.quantityRequested_ = limitRequest.quantity_, .status_ = SubmitStatus::REJECTED};
    }

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
    auto& idToOrderMap = impl.idToOrderMap_;

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
                    subResult.status_ = SubmitStatus::FILLED;

                    return subResult;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
            
                    subResult.quantityFilled_ += takingOrder->quantity_;
                    subResult.executions_.emplace_back(takingOrder->id_, topOfAsks->first, takingOrder->quantity_);
                    
                    idToOrderMap.erase(takingOrder->id_);

                    core::RestingOrder* clearedOrder = matchingLevel.pop_front();

                    impl.memoryPool_.deallocate(clearedOrder);

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
            subResult.status_ = SubmitStatus::FILLED;
        }
        else
        {
            RestingLifetime restingLifetime = (limitRequest.tif_ == TimeInForce::GTC) ? RestingLifetime::GTC : RestingLifetime::DAY;

            core::RestingOrder* newOrder = impl.memoryPool_.allocate(limitRequest.id_, remainingShares, restingLifetime);
            idToOrderMap.emplace(limitRequest.id_, newOrder);

            auto [it, inserted] = bidLevels.emplace(limitRequest.price_, core::PriceLevel{limitRequest.price_});
            it->second.push_back(newOrder);

            if (remainingShares > 0 && remainingShares < limitRequest.quantity_)
            {
                subResult.status_ = SubmitStatus::PARTIALLY_FILLED_RESTING;
            }
            else
            {
                subResult.status_ = SubmitStatus::RESTING;
            }
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
                    subResult.status_ = SubmitStatus::FILLED;

                    return subResult;
                }
                else
                {
                    remainingShares -= takingOrder->quantity_;
            
                    subResult.quantityFilled_ += takingOrder->quantity_;
                    subResult.executions_.emplace_back(takingOrder->id_, topOfBids->first, takingOrder->quantity_);
                    
                    idToOrderMap.erase(takingOrder->id_);

                    core::RestingOrder* clearedOrder = matchingLevel.pop_front();

                    impl.memoryPool_.deallocate(clearedOrder);

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
            subResult.status_ = SubmitStatus::FILLED;
        }
        else
        {
            RestingLifetime restingLifetime = (limitRequest.tif_ == TimeInForce::GTC) ? RestingLifetime::GTC : RestingLifetime::DAY;

            core::RestingOrder* newOrder = impl.memoryPool_.allocate(limitRequest.id_, remainingShares, restingLifetime);
            
            idToOrderMap.emplace(limitRequest.id_, newOrder);

            auto [it, inserted] = askLevels.emplace(limitRequest.price_, core::PriceLevel{limitRequest.price_});
            it->second.push_back(newOrder);

            if (remainingShares > 0 && remainingShares < limitRequest.quantity_)
            {
                subResult.status_ = SubmitStatus::PARTIALLY_FILLED_RESTING;
            }
            else
            {
                subResult.status_ = SubmitStatus::RESTING;
            }
        }
    }

    return subResult;
}

void submit_limit_order_fok();
void submit_limit_order_ioc();


template<typename LevelMap>
void OrderBook::prune_from_side_map(LevelMap& levelMap, DayOrderPruneResult& dayResult)
{
    for (auto it = levelMap.begin(); it != levelMap.end();)
    {
        core::PriceLevel& level = it->second;
        core::LevelPruneResult pruneResult = level.prune_day_orders();

        dayResult.ordersPruned += pruneResult.ordersPruned.size();
        dayResult.sharesErased += pruneResult.sharesErased;
        
        for (core::RestingOrder* order : pruneResult.ordersPruned)
        {
            retire_order(order);
        }

        if (level.empty())
        {
            it = levelMap.erase(it);
            dayResult.priceLevelsErased++;
        }
        else
        {
            it++;
        }
    }
}

DayOrderPruneResult OrderBook::on_session_end()
{
    auto& bidLevels = pImpl_->bidLevels_;
    auto& askLevels = pImpl_->askLevels_;

    DayOrderPruneResult sessionResult;

    prune_from_side_map(bidLevels, sessionResult);
    prune_from_side_map(askLevels, sessionResult);

    return sessionResult;
}

} // namespace lob