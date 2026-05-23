#pragma once

#include <cstddef>
#include <stdexcept>

#include "Aliases.hpp"

namespace lob {

struct OrderBookConfig {
    std::size_t initialPoolSize_;
    Price minPrice_;
    Price maxPrice_;

    OrderBookConfig() = delete;

    explicit OrderBookConfig(std::size_t initialPoolSize, Price minPrice, Price maxPrice)
        : initialPoolSize_{initialPoolSize}, minPrice_{minPrice}, maxPrice_{maxPrice} {
        if (minPrice_ > maxPrice_) {
            throw std::domain_error("Min price cannot be greater than max price.");
        }
    }
};

} // namespace lob