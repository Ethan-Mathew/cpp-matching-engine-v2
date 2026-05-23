#pragma once

#include <cstdint>

#include "Aliases.hpp"

namespace lob {

struct ExecutionResult {
    OrderID makerOrderID_;
    Price makerPrice_;
    Quantity executedQuantity_;
};

} // namespace lob