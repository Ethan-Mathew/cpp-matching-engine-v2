#include "lob/Aliases.hpp"
#include "lob/Side.hpp"

#include "PriceLadder.hpp"
#include "PriceLevel.hpp"

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace lob::core
{

PriceLadder::PriceLadder(Price minPrice, Price maxPrice, Side side)
    : minPrice_{minPrice},
      maxPrice_{maxPrice},
      side_{side},
      levels_(static_cast<std::size_t>(maxPrice - minPrice + 1)),
      bestPrice_{std::nullopt}
{
    assert(maxPrice_ >= minPrice_);
}

Side PriceLadder::get_side()
{
    return side_;
}

std::optional<Price> PriceLadder::get_best_price()
{
    return bestPrice_;
}

PriceLevel* PriceLadder::get_level_at_price(Price price)
{
    assert(price <= maxPrice_ && price >= minPrice_);

    return std::addressof(levels_[static_cast<std::size_t>(price - minPrice_)]);
}

void PriceLadder::set_best_price(Price price)
{
    assert(price <= maxPrice_ && price >= minPrice_);
    
    bestPrice_ = price;
}

void PriceLadder::update_best_price_from_given(Price startingPrice)
{
    assert(startingPrice <= maxPrice_ && startingPrice >= minPrice_);

    std::size_t startingIndex = static_cast<std::size_t>(startingPrice - minPrice_);

    if (side_ == Side::BUY)
    {
        std::size_t i = startingIndex;
        while (true)
        {
            if (!levels_[i].empty())
            {
                bestPrice_ = static_cast<Price>(i) + minPrice_;
                return;
            }

            if (i == 0)
            {
                break;
            }

            --i;
        }
    }
    else
    {
        std::size_t numLevels = levels_.size();
        for (std::size_t i = startingIndex; i < numLevels; i++)
        {
            if (!levels_[i].empty())
            {
                bestPrice_ = static_cast<Price>(i) + minPrice_;
                return;
            }
        }
    }

    bestPrice_ = std::nullopt;
    return;
}

bool PriceLadder::has_liquidity_at_price(Price price) const
{
    return !levels_[static_cast<std::size_t(price - minPrice_)].empty();
}

} // namespace lob::core