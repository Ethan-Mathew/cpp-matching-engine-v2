#pragma once

#include "Aliases.hpp"
#include "DayOrderPruneResult.hpp"
#include "Requests.hpp"
#include "Results.hpp"

#include <cstddef>
#include <memory>

namespace lob
{

class OrderBook
{
public:
    OrderBook(std::size_t poolSize);
    ~OrderBook();

    SubmissionResult submit_limit_order(const LimitOrderRequest& limitRequest);

    SubmissionResult submit_market_order(const MarketOrderRequest& marketRequest);

    void modify_order();
    void cancel_order();

    DayOrderPruneResult on_session_end();

    std::size_t get_num_orders() const;
    std::size_t get_num_levels_bids() const;
    std::size_t get_num_levels_asks() const;
    std::size_t get_memory_pool_size() const;
    std::size_t get_memory_pool_curr_alloc() const;
    std::size_t get_num_orders_at_level(Price level, Side side) const;
    std::size_t get_num_shares_at_level(Price level, Side side) const;
    bool check_level_exists(Price level, Side side) const;

private:
    template<Side S>
    bool crosses(Price orderPrice, Price levelPrice) const;

    template <Side S, typename LevelMap>
    bool check_available_liquidity(const LevelMap& levelMap, Price limitPrice, Quantity minimumQuantity) const;

    template<typename RestingOrderType>
    void retire_order(RestingOrderType* order);

    template<typename LevelMap>
    void prune_from_side_map(LevelMap& levelMap, DayOrderPruneResult& dayResult);

    template<Side S>
    SubmissionResult submit_limit_order_resting(const LimitOrderRequest& limitRequest);

    template<Side S>
    SubmissionResult submit_limit_order_ioc(const LimitOrderRequest& limitRequest);

    template<Side S>
    SubmissionResult submit_limit_order_fok(const LimitOrderRequest& limitRequest);
    
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace lob
