#include "lob/OrderBook.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/LevelPruneStats.hpp"
#include "core/MemoryPool.hpp"
#include "core/PriceLadder.hpp"
#include "core/PriceLevel.hpp"
#include "core/RestingLifetime.hpp"
#include "core/RestingOrder.hpp"
#include "lob/Aliases.hpp"
#include "lob/ExecutionResults.hpp"
#include "lob/OrderBookConfig.hpp"
#include "lob/OrderType.hpp"
#include "lob/Requests.hpp"
#include "lob/Results.hpp"
#include "lob/Side.hpp"
#include "lob/TimeInForce.hpp"

namespace lob {

OrderBook::OrderBook(const OrderBookConfig& config) : pImpl_{std::make_unique<Impl>(config)} {}

struct OrderBook::Impl {
    core::MemoryPool memoryPool_;
    std::unordered_map<OrderID, core::RestingOrder*> idToOrderMap_;

    core::PriceLadder bidLevels_;
    core::PriceLadder askLevels_;

    Price minPrice_;
    Price maxPrice_;

    Impl(const OrderBookConfig& config)
        : memoryPool_{config.initialPoolSize_},
          bidLevels_{config.minPrice_, config.maxPrice_, Side::BUY},
          askLevels_{config.minPrice_, config.maxPrice_, Side::SELL}, minPrice_{config.minPrice_},
          maxPrice_{config.maxPrice_} {}
};

OrderBook::~OrderBook() = default;

std::size_t OrderBook::get_num_orders() const { return pImpl_->idToOrderMap_.size(); }

std::size_t OrderBook::get_num_levels_bids() const {
    return pImpl_->bidLevels_.get_num_non_empty_levels();
}

std::size_t OrderBook::get_num_levels_asks() const {
    return pImpl_->askLevels_.get_num_non_empty_levels();
}

std::size_t OrderBook::get_memory_pool_size() const {
    return pImpl_->memoryPool_.get_total_elements();
}

std::size_t OrderBook::get_memory_pool_curr_alloc() const {
    return pImpl_->memoryPool_.get_currently_allocated();
}

std::size_t OrderBook::get_num_orders_at_level(Price level, Side side) const {
    if (side == Side::BUY) {
        return pImpl_->bidLevels_.get_level_at_price(level)->get_order_count();
    } else {
        return pImpl_->askLevels_.get_level_at_price(level)->get_order_count();
    }
}

std::size_t OrderBook::get_num_shares_at_level(Price level, Side side) const {
    if (side == Side::BUY) {
        return pImpl_->bidLevels_.get_level_at_price(level)->get_total_volume();
    } else {
        return pImpl_->askLevels_.get_level_at_price(level)->get_total_volume();
    }
}

std::optional<Price> OrderBook::get_best_bid_price() const {
    return pImpl_->bidLevels_.get_best_price();
}

std::optional<Price> OrderBook::get_best_ask_price() const {
    return pImpl_->askLevels_.get_best_price();
}

std::vector<std::pair<Price, Volume>> OrderBook::get_top_bid_levels(std::size_t depth) const {
    return pImpl_->bidLevels_.get_top_levels(depth);
}

std::vector<std::pair<Price, Volume>> OrderBook::get_top_ask_levels(std::size_t depth) const {
    return pImpl_->askLevels_.get_top_levels(depth);
}

bool OrderBook::check_level_exists(Price level, Side side) const {
    if (side == Side::BUY) {
        return !(pImpl_->bidLevels_.get_level_at_price(level)->empty());
    } else {
        return !(pImpl_->askLevels_.get_level_at_price(level)->empty());
    }
}

bool OrderBook::replay_add_visible_order(OrderID id, Price price, Quantity quantity, Side side) {
    Impl& impl = *pImpl_;

    if (quantity == 0 || impl.idToOrderMap_.contains(id) || price < impl.minPrice_ ||
        price > impl.maxPrice_) {
        return false;
    }

    core::RestingOrder* orderToRest =
        impl.memoryPool_.allocate(id, quantity, core::RestingLifetime::GTC, side);

    core::PriceLadder& restingLevels = (side == Side::BUY) ? impl.bidLevels_ : impl.askLevels_;

    core::PriceLevel& restingLevel = *restingLevels.get_level_at_price(price);

    restingLevel.push_back(orderToRest);
    impl.idToOrderMap_.emplace(id, orderToRest);

    const std::optional<Price> currentBestPrice = restingLevels.get_best_price();

    if (!currentBestPrice.has_value() || (side == Side::BUY && price > *currentBestPrice) ||
        (side == Side::SELL && price < *currentBestPrice)) {
        restingLevels.set_best_price(price);
    }

    return true;
}

bool OrderBook::replay_reduce_visible_order(OrderID id, Quantity quantityReduced) {
    Impl& impl = *pImpl_;

    if (quantityReduced == 0) {
        return false;
    }

    auto it = impl.idToOrderMap_.find(id);

    if (it == impl.idToOrderMap_.end()) {
        return false;
    }

    core::RestingOrder* order = it->second;

    if (quantityReduced > order->quantity_) {
        return false;
    }

    if (quantityReduced == order->quantity_) {
        return replay_delete_visible_order(id);
    }

    core::PriceLevel* level = order->level_;
    assert(level);

    level->reduce_order_quantity(order, quantityReduced);

    return true;
}

bool OrderBook::replay_delete_visible_order(OrderID id) {
    const CancelResult cancelResult = cancel_order(CancelOrderRequest{id});

    return cancelResult.status_ == CancelStatus::CANCELED;
}

bool OrderBook::replay_replace_visible_order(OrderID originalId, OrderID newId, Price newPrice,
                                             Quantity newQuantity) {
    Impl& impl = *pImpl_;

    if (newQuantity == 0 || newPrice < impl.minPrice_ || newPrice > impl.maxPrice_ ||
        impl.idToOrderMap_.contains(newId)) {
        return false;
    }

    auto it = impl.idToOrderMap_.find(originalId);

    if (it == impl.idToOrderMap_.end()) {
        return false;
    }

    const Side originalSide = it->second->side_;

    if (!replay_delete_visible_order(originalId)) {
        return false;
    }

    return replay_add_visible_order(newId, newPrice, newQuantity, originalSide);
}

template <Side S> bool OrderBook::crosses(Price orderPrice, Price levelPrice) const {
    if constexpr (S == Side::BUY) {
        return orderPrice >= levelPrice;
    } else {
        return orderPrice <= levelPrice;
    }
}

template <Side S, typename LevelMap>
bool OrderBook::check_available_liquidity(const LevelMap& levelMap, Price limitPrice,
                                          Quantity minimumQuantity) const {
    Volume totalValidVolume = 0;

    for (const auto& [levelPrice, levelPtr] : levelMap) {
        if (totalValidVolume >= minimumQuantity || crosses<S>(limitPrice, levelPrice)) {
            totalValidVolume += levelPtr.get_total_volume();
        } else {
            break;
        }
    }

    return totalValidVolume >= minimumQuantity;
}

template <typename RestingOrderType> void OrderBook::retire_order(RestingOrderType* order) {
    pImpl_->idToOrderMap_.erase(order->id_);
    pImpl_->memoryPool_.deallocate(order);
}

SubmissionResult OrderBook::submit_limit_order(const LimitOrderRequest& limitRequest) {
    auto& idToOrderMap = pImpl_->idToOrderMap_;

    if (idToOrderMap.contains(limitRequest.id_) || limitRequest.price_ > pImpl_->maxPrice_ ||
        limitRequest.price_ < pImpl_->minPrice_) {
        return SubmissionResult{.quantityRequested_ = limitRequest.quantity_,
                                .status_ = SubmitStatus::REJECTED};
    }

    switch (limitRequest.tif_) {
    case TimeInForce::GTC:
    case TimeInForce::DAY:
        if (limitRequest.side_ == Side::BUY) {
            return submit_limit_order_resting<Side::BUY>(limitRequest);
        } else {
            return submit_limit_order_resting<Side::SELL>(limitRequest);
        }
    case TimeInForce::IOC:
        if (limitRequest.side_ == Side::BUY) {
            return submit_limit_order_ioc<Side::BUY>(limitRequest);
        } else {
            return submit_limit_order_ioc<Side::SELL>(limitRequest);
        }
    case TimeInForce::FOK:
        if (limitRequest.side_ == Side::BUY) {
            return submit_limit_order_fok<Side::BUY>(limitRequest);
        } else {
            return submit_limit_order_fok<Side::SELL>(limitRequest);
        };
    }

    return SubmissionResult{.quantityRequested_ = limitRequest.quantity_,
                            .status_ = SubmitStatus::REJECTED};
}

SubmissionResult OrderBook::submit_market_order(const MarketOrderRequest& marketRequest) {
    Impl& impl = *pImpl_;
    auto& idToOrderMap = impl.idToOrderMap_;

    if (idToOrderMap.contains(marketRequest.id_)) {
        return SubmissionResult{.quantityRequested_ = marketRequest.quantity_,
                                .status_ = SubmitStatus::REJECTED};
    }

    core::PriceLadder& matchingLevels =
        (marketRequest.side_ == Side::BUY) ? impl.askLevels_ : impl.bidLevels_;

    Quantity remainingShares = marketRequest.quantity_;
    SubmissionResult subResult{.quantityRequested_ = marketRequest.quantity_};

    while (remainingShares > 0) {
        std::optional<Price> bestPrice = matchingLevels.get_best_price();

        if (bestPrice.has_value()) {
            core::PriceLevel& matchingLevel = *(matchingLevels.get_level_at_price(*bestPrice));

            core::RestingOrder* takingOrder = matchingLevel.front();
            OrderID takingOrderID = takingOrder->id_;
            Quantity takingOrderQuantity = takingOrder->quantity_;

            if (takingOrderQuantity > remainingShares) {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, remainingShares);
                matchingLevel.take_shares_from_first(remainingShares);

                subResult.quantityFilled_ += remainingShares;
                subResult.status_ = SubmitStatus::FILLED;

                return subResult;
            } else {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, takingOrderQuantity);
                subResult.quantityFilled_ += takingOrderQuantity;

                idToOrderMap.erase(takingOrderID);

                remainingShares -= takingOrderQuantity;

                core::RestingOrder* clearedOrder = matchingLevel.pop_front();
                impl.memoryPool_.deallocate(clearedOrder);

                if (matchingLevel.empty()) {
                    matchingLevels.update_best_price_from_given(*bestPrice);
                }
            }
        } else {
            break;
        }
    }

    if (remainingShares == 0) {
        subResult.status_ = SubmitStatus::FILLED;
    } else if (remainingShares < marketRequest.quantity_) {
        subResult.status_ = SubmitStatus::PARTIALLY_FILLED_CANCELED;
    } else {
        subResult.status_ = SubmitStatus::CANCELED;
    }

    return subResult;
}

CancelResult OrderBook::cancel_order(const CancelOrderRequest& cancelRequest) {
    Impl& impl = *pImpl_;
    auto& idToOrderMap = impl.idToOrderMap_;

    auto it = idToOrderMap.find(cancelRequest.id_);

    if (it == idToOrderMap.end()) {
        return CancelResult{.status_ = CancelStatus::NOT_FOUND};
    }

    core::RestingOrder* cancelOrder = it->second;
    core::PriceLevel* cancellationLevel = cancelOrder->level_;
    cancellationLevel->remove_order(cancelOrder);

    if (cancellationLevel->empty()) {
        core::PriceLadder& restingLevels =
            (cancelOrder->side_ == Side::BUY) ? impl.bidLevels_ : impl.askLevels_;

        Price cancelPrice = cancellationLevel->get_price();
        std::optional<Price> currentBestPrice = restingLevels.get_best_price();

        if (currentBestPrice.has_value() && *currentBestPrice == cancelPrice) {
            restingLevels.update_best_price_from_given(cancelPrice);
        }
    }

    Quantity quantityCancelled = cancelOrder->quantity_;

    retire_order(cancelOrder);

    return CancelResult{quantityCancelled, CancelStatus::CANCELED};
}

ModificationResult OrderBook::modify_order(const ModifyOrderRequest& modificationRequest) {
    Impl& impl = *pImpl_;
    auto& idToOrderMap = impl.idToOrderMap_;

    if (modificationRequest.newPrice_ > impl.maxPrice_ ||
        modificationRequest.newPrice_ < impl.minPrice_) {
        return ModificationResult{.status_ = ModificationStatus::REJECTED,
                                  .resubmissionResult_ = std::nullopt};
    }

    auto it = idToOrderMap.find(modificationRequest.id_);

    if (it == idToOrderMap.end()) {
        return ModificationResult{.status_ = ModificationStatus::NOT_FOUND,
                                  .resubmissionResult_ = std::nullopt};
    }

    const core::RestingOrder* const resubmitOrder = it->second;
    const Quantity originalQuantity = resubmitOrder->quantity_;
    const Side originalSide = resubmitOrder->side_;
    const TimeInForce originalLifetime = (resubmitOrder->lifetime_ == core::RestingLifetime::GTC)
                                             ? TimeInForce::GTC
                                             : TimeInForce::DAY;

    const CancelOrderRequest cancelRequest{modificationRequest.id_};
    const CancelResult cancelResult = cancel_order(cancelRequest);

    if (modificationRequest.newQuantity_ > 0) {
        const LimitOrderRequest limitRequest{modificationRequest.id_, modificationRequest.newPrice_,
                                             modificationRequest.newQuantity_, originalSide,
                                             originalLifetime};

        const SubmissionResult resubmitResult = submit_limit_order(limitRequest);

        return ModificationResult{originalQuantity, ModificationStatus::RESUBMITTED,
                                  resubmitResult};
    } else {
        return ModificationResult{cancelResult.quantityCancelled_, ModificationStatus::CANCELED,
                                  std::nullopt};
    }
}

template <Side S>
SubmissionResult OrderBook::submit_limit_order_resting(const LimitOrderRequest& limitRequest) {
    Impl& impl = *pImpl_;
    core::PriceLadder& matchingLevels = (S == Side::BUY) ? impl.askLevels_ : impl.bidLevels_;
    auto& idToOrderMap = impl.idToOrderMap_;

    Quantity remainingShares = limitRequest.quantity_;
    SubmissionResult subResult{.quantityRequested_ = limitRequest.quantity_};

    while (remainingShares > 0) {
        std::optional<Price> bestPrice = matchingLevels.get_best_price();

        if (bestPrice.has_value() && crosses<S>(limitRequest.price_, *bestPrice)) {
            core::PriceLevel& matchingLevel = *(matchingLevels.get_level_at_price(*bestPrice));

            core::RestingOrder* takingOrder = matchingLevel.front();
            OrderID takingOrderID = takingOrder->id_;
            Quantity takingOrderQuantity = takingOrder->quantity_;

            if (takingOrderQuantity > remainingShares) {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, remainingShares);
                matchingLevel.take_shares_from_first(remainingShares);

                subResult.quantityFilled_ += remainingShares;
                subResult.status_ = SubmitStatus::FILLED;

                return subResult;
            } else {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, takingOrderQuantity);
                subResult.quantityFilled_ += takingOrderQuantity;

                idToOrderMap.erase(takingOrderID);

                remainingShares -= takingOrderQuantity;

                core::RestingOrder* clearedOrder = matchingLevel.pop_front();
                impl.memoryPool_.deallocate(clearedOrder);

                if (matchingLevel.empty()) {
                    matchingLevels.update_best_price_from_given(*bestPrice);
                }
            }
        } else {
            break;
        }
    }

    if (remainingShares == 0) {
        subResult.status_ = SubmitStatus::FILLED;
        return subResult;
    }

    core::RestingOrder* orderToRest = impl.memoryPool_.allocate(
        limitRequest.id_, remainingShares,
        (limitRequest.tif_ == TimeInForce::GTC) ? core::RestingLifetime::GTC
                                                : core::RestingLifetime::DAY,
        limitRequest.side_);

    core::PriceLadder& restingLevels = (S == Side::BUY) ? impl.bidLevels_ : impl.askLevels_;
    core::PriceLevel& restingLevel = *(restingLevels.get_level_at_price(limitRequest.price_));

    restingLevel.push_back(orderToRest);

    idToOrderMap.emplace(limitRequest.id_, orderToRest);

    std::optional<Price> currentBestPrice = restingLevels.get_best_price();

    if (!currentBestPrice.has_value() ||
        (restingLevels.get_side() == Side::BUY && limitRequest.price_ > *currentBestPrice) ||
        (restingLevels.get_side() == Side::SELL && limitRequest.price_ < *currentBestPrice)) {
        restingLevels.set_best_price(limitRequest.price_);
    }

    if (remainingShares < limitRequest.quantity_) {
        subResult.status_ = SubmitStatus::PARTIALLY_FILLED_RESTING;
    } else {
        subResult.status_ = SubmitStatus::RESTING;
    }

    return subResult;
}

template <Side S>
SubmissionResult OrderBook::submit_limit_order_ioc(const LimitOrderRequest& limitRequest) {
    Impl& impl = *pImpl_;
    core::PriceLadder& matchingLevels = (S == Side::BUY) ? impl.askLevels_ : impl.bidLevels_;
    auto& idToOrderMap = impl.idToOrderMap_;

    Quantity remainingShares = limitRequest.quantity_;
    SubmissionResult subResult{.quantityRequested_ = limitRequest.quantity_};

    while (remainingShares > 0) {
        std::optional<Price> bestPrice = matchingLevels.get_best_price();

        if (bestPrice.has_value() && crosses<S>(limitRequest.price_, *bestPrice)) {
            core::PriceLevel& matchingLevel = *(matchingLevels.get_level_at_price(*bestPrice));

            core::RestingOrder* takingOrder = matchingLevel.front();
            OrderID takingOrderID = takingOrder->id_;
            Quantity takingOrderQuantity = takingOrder->quantity_;

            if (takingOrderQuantity > remainingShares) {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, remainingShares);
                matchingLevel.take_shares_from_first(remainingShares);

                subResult.quantityFilled_ += remainingShares;
                subResult.status_ = SubmitStatus::FILLED;

                return subResult;
            } else {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, takingOrderQuantity);
                subResult.quantityFilled_ += takingOrderQuantity;

                idToOrderMap.erase(takingOrderID);

                remainingShares -= takingOrderQuantity;

                core::RestingOrder* clearedOrder = matchingLevel.pop_front();
                impl.memoryPool_.deallocate(clearedOrder);

                if (matchingLevel.empty()) {
                    matchingLevels.update_best_price_from_given(*bestPrice);
                }
            }
        } else {
            break;
        }
    }

    if (remainingShares == 0) {
        subResult.status_ = SubmitStatus::FILLED;
    } else if (remainingShares < limitRequest.quantity_) {
        subResult.status_ = SubmitStatus::PARTIALLY_FILLED_CANCELED;
    } else {
        subResult.status_ = SubmitStatus::CANCELED;
    }

    return subResult;
}

template <Side S>
SubmissionResult OrderBook::submit_limit_order_fok(const LimitOrderRequest& limitRequest) {
    Impl& impl = *pImpl_;
    core::PriceLadder& matchingLevels = (S == Side::BUY) ? impl.askLevels_ : impl.bidLevels_;
    auto& idToOrderMap = impl.idToOrderMap_;

    SubmissionResult subResult{.quantityRequested_ = limitRequest.quantity_};

    if (!matchingLevels.has_sufficient_marketable_liquidity(limitRequest.price_,
                                                            limitRequest.quantity_)) {
        subResult.status_ = SubmitStatus::KILLED;
        return subResult;
    }

    Quantity remainingShares = limitRequest.quantity_;

    while (remainingShares > 0) {
        std::optional<Price> bestPrice = matchingLevels.get_best_price();

        if (bestPrice.has_value() && crosses<S>(limitRequest.price_, *bestPrice)) {
            core::PriceLevel& matchingLevel = *(matchingLevels.get_level_at_price(*bestPrice));

            core::RestingOrder* takingOrder = matchingLevel.front();
            OrderID takingOrderID = takingOrder->id_;
            Quantity takingOrderQuantity = takingOrder->quantity_;

            if (takingOrderQuantity > remainingShares) {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, remainingShares);
                matchingLevel.take_shares_from_first(remainingShares);

                subResult.quantityFilled_ += remainingShares;
                subResult.status_ = SubmitStatus::FILLED;

                return subResult;
            } else {
                subResult.executions_.emplace_back(takingOrderID, *bestPrice, takingOrderQuantity);
                subResult.quantityFilled_ += takingOrderQuantity;

                idToOrderMap.erase(takingOrderID);

                remainingShares -= takingOrderQuantity;

                core::RestingOrder* clearedOrder = matchingLevel.pop_front();
                impl.memoryPool_.deallocate(clearedOrder);

                if (matchingLevel.empty()) {
                    matchingLevels.update_best_price_from_given(*bestPrice);
                }
            }
        } else {
            break;
        }
    }

    assert(remainingShares == 0);

    subResult.status_ = SubmitStatus::FILLED;

    return subResult;
}

DayOrderPruneResult OrderBook::on_session_end() {
    auto retireOrder = [&](core::RestingOrder* order) { retire_order(order); };

    DayOrderPruneResult sessionResult;

    if (!pImpl_->bidLevels_.empty()) {
        pImpl_->bidLevels_.prune_day_orders(sessionResult, retireOrder);
    }

    if (!pImpl_->askLevels_.empty()) {
        pImpl_->askLevels_.prune_day_orders(sessionResult, retireOrder);
    }

    return sessionResult;
}

} // namespace lob