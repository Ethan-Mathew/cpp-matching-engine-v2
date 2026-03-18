#include "lob/Aliases.hpp"

#include "PriceLevel.hpp"
#include "RestingOrder.hpp"

#include <cstdint>

using namespace lob::core;

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
    newOrder->level_ = this;

    totalVolume_ += newOrder->quantity_;
    orderCount_++;
}

RestingOrder* PriceLevel::pop_front()
{
    if (!head_) return nullptr; 

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

    totalVolume_ -= retOrder->quantity_;
    orderCount_--;

    return retOrder;
}

bool PriceLevel::remove_order(RestingOrder* order)
{
    if (order == head_)
    {
        head_ = order->next_;
        head_->prev_ = nullptr;
        
        order->next_ = nullptr;
    }
    else if (order == tail_)
    {
        tail_ = order->prev_;
        tail_->next_ = nullptr;
        
        order->prev_ = nullptr;
    }
    else
    {
        RestingOrder* prev = order->prev_;
        RestingOrder* next = order->next_;
        
        prev->next_ = next;
        next->prev_ = prev;

        order->next_ = nullptr;
        order->prev_ = nullptr;
    }

    order->level_ = nullptr;

    totalVolume_ -= order->quantity_;
    orderCount_--;

    return true;
}

RestingOrder* PriceLevel::front()
{
    return head_;
}

const RestingOrder* PriceLevel::front() const
{
    return front();
}

bool PriceLevel::empty() const
{
    return orderCount_ == 0;
}

lob::Volume PriceLevel::get_total_volume() const
{
    return totalVolume_;
}

std::uint32_t PriceLevel::get_order_count() const
{
    return orderCount_;
}