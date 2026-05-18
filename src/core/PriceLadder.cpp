#include "lob/Aliases.hpp"
#include "lob/Side.hpp"

#include "LevelPruneStats.hpp"
#include "PriceLadder.hpp"
#include "PriceLevel.hpp"

#include <algorithm>
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

std::size_t PriceLadder::get_num_non_empty_levels()
{
    std::size_t nonEmptyLevelsCount = 0;

    std::for_each(levels_.begin(), levels_.end(), [this, &nonEmptyLevelsCount](PriceLevel& level)
    {
        if (!level.empty())
        {
            ++nonEmptyLevelsCount;
        }
    });

    return nonEmptyLevelsCount;
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

bool PriceLadder::has_sufficient_marketable_liquidity(Price thresholdPrice,
                                                      Quantity requiredLiquidity
                                                     ) const
{
    assert(requiredLiquidity > 0);
    assert(thresholdPrice <= maxPrice && thresholdPrice >= minPrice);

    if (!bestPrice_.has_value())
    {
        return false;
    }

    const Price bestPrice = *bestPrice_;
    if ((side_ == Side::BUY && bestPrice < thresholdPrice) ||
        (side_ == Side::SELL && bestPrice > thresholdPrice)
       )
    {
        return false;
    }

    const std::size_t thresholdIndex = static_cast<std::size_t>(thresholdPrice - minPrice_);
    const std::size_t bestIndex = static_cast<std::size_t>(bestPrice - minPrice_);

    Volume availableLiquidity = 0;

    if (side_ == Side::BUY)
    {
        std::size_t i = bestIndex;

        while (true)
        {
            if (!levels_[i].empty())
            {
                availableLiquidity += levels_[i].get_total_volume();

                if (availableLiquidity >= static_cast<Volume>(requiredLiquidity))
                {
                    return true;
                }
            }

            if (i == thresholdIndex)
            {
                break;
            }

            --i;
        }
    }
    else
    {
        for (std::size_t i = bestIndex; i <= thresholdIndex; ++i)
        {
            if (!levels_[i].empty())
            {
                availableLiquidity += levels_[i].get_total_volume();

                if (availableLiquidity >= static_cast<Volume>(requiredLiquidity))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool PriceLadder::has_liquidity_at_price(Price price) const
{
    return !levels_[static_cast<std::size_t>(price - minPrice_)].empty();
}

} // namespace lob::core