#pragma once

#include "lob/Aliases.hpp"

#include "LevelPruneStats.hpp"
#include "RestingOrder.hpp"

#include <cstdint>

namespace lob::core
{

class PriceLevel
{
public:

    enum class RemoveOrderResult : std::uint8_t
    {
        EMPTY,
        NON_EMPTY
    };

    PriceLevel() = delete;

    explicit PriceLevel(lob::Price price);

    void push_back(RestingOrder* newOrder);

    RestingOrder* pop_front();

    RemoveOrderResult remove_order(RestingOrder* order);

    void take_shares_from_first(Quantity sharesTaken);

    template <typename OnPrunedOrder>
    LevelPruneStats prune_day_orders(OnPrunedOrder&& onPrunedOrder);

    RestingOrder* front();
    const RestingOrder* front() const;
    bool empty() const;

    Price get_price() const;
    Volume get_total_volume() const;
    std::uint32_t get_order_count() const;

private:

    RestingOrder* head_ = nullptr;
    RestingOrder* tail_ = nullptr;
    
    Price price_;
    Volume totalVolume_  = 0;
    std::uint32_t orderCount_ = 0;
};

template <typename OnPrunedOrder>
LevelPruneStats PriceLevel::prune_day_orders(OnPrunedOrder&& onPrunedOrder)
{
    LevelPruneStats result;

    RestingOrder* ptr = head_;

    while (ptr)
    {
        RestingOrder* next = ptr->next_;

        if (ptr->lifetime_ == RestingLifetime::DAY)
        {
            result.quantityPruned_ += ptr->quantity_;
            result.ordersPruned_++;

            remove_order(ptr);
            onPrunedOrder(ptr);
        }

        ptr = next;
    }

    return result;
}

} // namespace lob::core