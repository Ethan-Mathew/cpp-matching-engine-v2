#pragma once

#include "lob/Aliases.hpp"

#include <cstdint>

struct RestingOrder;

class PriceLevel
{
public:
    PriceLevel() = delete;

    explicit PriceLevel(lob::Price price);

    void push_back(RestingOrder* newOrder);
    bool remove_order(RestingOrder* order);
    RestingOrder* pop_front();

    RestingOrder* front();
    const RestingOrder* front() const;
    bool empty() const;

    lob::Volume get_total_volume() const;
    std::uint32_t get_order_count() const;

private:
    RestingOrder* head_ = nullptr;
    RestingOrder* tail_ = nullptr;
    
    lob::Price price_;
    lob::Volume totalVolume_  = 0;
    std::uint32_t orderCount_ = 0;
};
