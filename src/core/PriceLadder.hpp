#pragma once

#include "lob/Aliases.hpp"

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

    void set_best_price(Price price);
    void update_best_price_from_given(Price startingPrice);

    bool has_liquidity_at_price(Price price) const;
    bool empty() const;

private:
    Price minPrice_;
    Price maxPrice_;
    Side side_;

    std::vector<PriceLevel> levels_;
    std::optional<Price> bestPrice_;
};

} // namespace lob::core