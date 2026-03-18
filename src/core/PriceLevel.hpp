#pragma once

#include "lob/Aliases.hpp"

#include <cstdint>

namespace lob::core
{

struct RestingOrder;

class PriceLevel
{
public:
    PriceLevel() = delete;

    explicit PriceLevel(lob::Price price);

    void push_back(RestingOrder* newOrder);
    RestingOrder* pop_front();
    bool remove_order(RestingOrder* order);

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

} // namespace lob::core