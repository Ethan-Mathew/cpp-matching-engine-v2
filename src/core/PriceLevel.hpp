#pragma once

#include "lob/Aliases.hpp"

#include "RestingOrder.hpp"

#include <cstdint>

namespace lob::core
{

enum class RemoveOrderResult : std::uint8_t
{
    EMPTY,
    NON_EMPTY
};

class PriceLevel
{
public:
    PriceLevel() = delete;

    explicit PriceLevel(lob::Price price);

    void push_back(RestingOrder* newOrder);

    RestingOrder* pop_front();

    //[[nodiscard("Resultant state of price level after removal should be used.")]]
    RemoveOrderResult remove_order(RestingOrder* order);

    void take_shares(Quantity sharesTaken);
    void take_all_shares();

    RestingOrder* front();
    const RestingOrder* front() const;
    bool empty() const;

    Volume get_total_volume() const;
    std::uint32_t get_order_count() const;

private:
    RestingOrder* head_ = nullptr;
    RestingOrder* tail_ = nullptr;
    
    Price price_;
    Volume totalVolume_  = 0;
    std::uint32_t orderCount_ = 0;
};

} // namespace lob::core