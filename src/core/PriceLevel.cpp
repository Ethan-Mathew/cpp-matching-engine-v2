#include "lob/Aliases.hpp"

#include "LevelPruneResult.hpp"
#include "PriceLevel.hpp"
#include "RestingOrder.hpp"

#include <cassert>
#include <cstdint>

namespace lob::core
{

PriceLevel::PriceLevel(lob::Price price)
    : price_{price}
{
}

void PriceLevel::push_back(RestingOrder* newOrder)
{
    if (tail_)
    {
        newOrder->prev_ = tail_;
        tail_->next_ = newOrder;
    }
    else
    {
        head_ = newOrder;
    }

    tail_ = newOrder;
    newOrder->next_ = nullptr;
    newOrder->level_ = this;

    totalVolume_ += newOrder->quantity_;
    orderCount_++;
}

RestingOrder* PriceLevel::pop_front()
{
    if (!head_)
    {
        return nullptr; 
    }

    RestingOrder* retOrder = head_;

    if (head_->next_)
    {
        head_ = head_->next_;
        head_->prev_ = nullptr;
    }
    else
    {
        head_ = nullptr;
        tail_ = nullptr;
    }

    retOrder->next_ = nullptr;
    retOrder->prev_ = nullptr;
    retOrder->level_ = nullptr;

    totalVolume_ -= retOrder->quantity_;
    orderCount_--;

    return retOrder;
}

PriceLevel::RemoveOrderResult PriceLevel::remove_order(RestingOrder* order)
{
    assert(head_);
    assert(order);
    assert(order->level_ == this);

    RestingOrder* prev = order->prev_;
    RestingOrder* next = order->next_;

    if (prev)
    {
        prev->next_ = next;
    }
    else
    {
        head_ = next;
    }

    if (next)
    {
        next->prev_ = prev;
    }
    else
    {
        tail_ = prev;
    }

    order->next_ = nullptr;
    order->prev_ = nullptr;
    order->level_ = nullptr;

    totalVolume_ -= order->quantity_;
    orderCount_--;

    return empty() ? RemoveOrderResult::EMPTY : RemoveOrderResult::NON_EMPTY;
}

void PriceLevel::take_shares_from_first(Quantity sharesTaken)
{
    head_->quantity_ -= sharesTaken;
    totalVolume_ -= sharesTaken;
}

LevelPruneResult PriceLevel::prune_day_orders()
{
    LevelPruneResult result;
    RestingOrder* ptr = head_;

    while (ptr)
    {
        RestingOrder* next = ptr->next_;

        if (ptr->lifetime_ == RestingLifetime::DAY)
        {
            result.sharesErased += ptr->quantity_;
            result.ordersPruned.push_back(ptr);

            remove_order(ptr);
        }

        ptr = next;
    }

    return result;
}

RestingOrder* PriceLevel::front()
{
    return head_;
}

const RestingOrder* PriceLevel::front() const
{
    return head_;
}

bool PriceLevel::empty() const
{
    return orderCount_ == 0;
}

Price PriceLevel::get_price() const
{
    return price_;
}

Volume PriceLevel::get_total_volume() const
{
    return totalVolume_;
}

std::uint32_t PriceLevel::get_order_count() const
{
    return orderCount_;
}

} // namespace lob::core