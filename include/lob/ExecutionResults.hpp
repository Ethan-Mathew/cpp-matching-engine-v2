#pragma once

#include "Aliases.hpp"

#include <cstdint>

namespace lob
{

struct ExecutionResult
{
    OrderID makerOrderID_;
    Price makerPrice_;
    Quantity executedQuantity_;
};

} // namespace lob