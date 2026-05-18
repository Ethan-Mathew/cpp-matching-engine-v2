#pragma once

#include "lob/Aliases.hpp"
#include "lob/Results.hpp"

#include "PriceLevel.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace lob::core
{

class PriceLadder
{
public:
    explicit PriceLadder(Price minPrice, Price maxPrice, Side side);

    Side get_side();
    std::optional<Price> get_best_price();
    PriceLevel* get_level_at_price(Price price);
    std::size_t get_num_non_empty_levels();

    void set_best_price(Price price);
    void update_best_price_from_given(Price startingPrice);
    template <typename OnPrunedOrder>
    void prune_day_orders(DayOrderPruneResult& dayResult, OnPrunedOrder&& onPrunedOrder);

    bool has_sufficient_marketable_liquidity(Price thresholdPrice, 
                                             Quantity neededLiquidity
                                            ) const;
    bool has_liquidity_at_price(Price price) const;
    bool empty() const;

private:
    const Price minPrice_;
    const Price maxPrice_;
    const Side side_;

    std::vector<PriceLevel> levels_;
    std::optional<Price> bestPrice_;
};

template <typename OnPrunedOrder>
void PriceLadder::prune_day_orders(DayOrderPruneResult& dayResult, OnPrunedOrder&& onPrunedOrder)
{
    for (std::size_t i = 0; i < levels_.size(); ++i)
    {
        if (!levels_[i].empty())
        {
            core::PriceLevel& level = levels_[i];

            const core::LevelPruneStats levelStats = level.prune_day_orders([&](core::RestingOrder* order)
                                                     {
                                                         onPrunedOrder(order);
                                                     });

            dayResult.ordersPruned += levelStats.ordersPruned_;
            dayResult.sharesErased += levelStats.quantityPruned_;

            if (level.empty())
            {
                ++dayResult.priceLevelsErased;
            }
        }
    }

    if (bestPrice_.has_value())
    {
        core::PriceLevel& currentBestPriceLevel = *get_level_at_price(*bestPrice_);

        if (currentBestPriceLevel.empty())
        {
            update_best_price_from_given(*bestPrice_);
        }
    }
}

} // namespace lob::core