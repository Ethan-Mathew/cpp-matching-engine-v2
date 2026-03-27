#include "Aliases.hpp"
#include "ExecutionResults.hpp"
#include "OrderRequests.hpp"
#include "PriceLevel.hpp"
#include "RestingOrder.hpp"
#include "UserSubmissionResults.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

namespace lob
{

class OrderBook
{
public:

    struct ExecutionResult
    {
        OrderID makerOrderID_;
        Price makerPrice;
        Quantity executedQuantity;
    };

    OrderBook() = default;

    core::SubmissionResult submit_limit_order(const LimitOrderRequest& limitRequest);
    void submit_market_order();
    void modify_order();
    void cancel_order();
    void prune_good_for_day_orders();

private:
    template<Side S>
    bool crosses(Price orderPrice, Price levelPrice) const;

    void submit_limit_order_fok();
    void submit_limit_order_ioc();

    template<Side S>
    core::SubmissionResult submit_limit_order_resting(const LimitOrderRequest& limitRequest);
  
    std::unordered_map<OrderID, lob::core::RestingOrder*> idToOrderMap;
    std::map<Price, lob::core::PriceLevel, std::greater<Price>> bidLevels_;
    std::map<Price, lob::core::PriceLevel, std::less<Price>> askLevels_;
};

} // namespace lob::core
