#include "ItchReplayEngine.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <optional>
#include <ostream>
#include <string>

#include "data/CommonFieldAliases.hpp"
#include "data/DecodedMessageTypes.hpp"

namespace lob::replay::engine {

ItchReplayEngine::ItchReplayEngine(const std::string& symbol, const OrderBookConfig& obConfig)
    : symbol_{symbol}, targetStockLocate_{std::nullopt}, ob_{obConfig} {}

void ItchReplayEngine::on_message(const data::SystemEventMessage&) {}

void ItchReplayEngine::on_message(const data::StockDirectoryMessage& message) {
    if (matches_symbol(message.stock_, symbol_)) {
        targetStockLocate_ = message.stockLocate_;
    }
}

void ItchReplayEngine::on_message(const data::AddOrderMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied =
        ob_.replay_add_visible_order(message.orderReferenceNumber_, message.price_, message.shares_,
                                     (message.buySellIndicator_ == 'B') ? Side::BUY : Side::SELL);

    if (applied) {
        ++addsApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::AddOrderWithMPIDMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied =
        ob_.replay_add_visible_order(message.orderReferenceNumber_, message.price_, message.shares_,
                                     (message.buySellIndicator_ == 'B') ? Side::BUY : Side::SELL);

    if (applied) {
        ++addsApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::OrderExecutedMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied =
        ob_.replay_reduce_visible_order(message.orderReferenceNumber_, message.executedShares_);

    if (applied) {
        ++executionsApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::OrderExecutedWithPriceMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied =
        ob_.replay_reduce_visible_order(message.orderReferenceNumber_, message.executedShares_);

    if (applied) {
        ++executionsApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::OrderCancelMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied =
        ob_.replay_reduce_visible_order(message.orderReferenceNumber_, message.cancelledShares_);

    if (applied) {
        ++cancelsApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::OrderDeleteMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied = ob_.replay_delete_visible_order(message.orderReferenceNumber_);

    if (applied) {
        ++deletesApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::on_message(const data::OrderReplaceMessage& message) {
    if (!is_target_stock(message.stockLocate_)) {
        return;
    }

    const bool applied = ob_.replay_replace_visible_order(message.originalOrderReferenceNumber_,
                                                          message.newOrderReferenceNumber_,
                                                          message.price_, message.shares_);

    if (applied) {
        ++replacesApplied_;
    } else {
        ++failedApplications_;
    }
}

void ItchReplayEngine::print_summary(std::ostream& out, std::size_t depth) const {
    out << "\nITCH Replay Summary\n";
    out << "-------------------\n";
    out << "Symbol: " << symbol_ << '\n';

    if (targetStockLocate_.has_value()) {
        out << "Stock Locate: " << *targetStockLocate_ << '\n';
    } else {
        out << "Stock Locate: not found\n";
    }

    out << "\nApplied target-symbol messages:\n";
    out << "Adds:        " << addsApplied_ << '\n';
    out << "Executions:  " << executionsApplied_ << '\n';
    out << "Cancels:     " << cancelsApplied_ << '\n';
    out << "Deletes:     " << deletesApplied_ << '\n';
    out << "Replaces:    " << replacesApplied_ << '\n';
    out << "Failures:    " << failedApplications_ << '\n';

    out << "\nFinal reconstructed visible book:\n";
    out << "Active orders:      " << ob_.get_num_orders() << '\n';
    out << "Active bid levels:  " << ob_.get_num_levels_bids() << '\n';
    out << "Active ask levels:  " << ob_.get_num_levels_asks() << '\n';

    const std::optional<Price> bestBid = ob_.get_best_bid_price();
    const std::optional<Price> bestAsk = ob_.get_best_ask_price();

    out << "\nTop of book:\n";
    out << std::fixed << std::setprecision(4);

    if (bestBid.has_value()) {
        out << "Best Bid: " << static_cast<double>(*bestBid) / 10'000 << " x "
            << ob_.get_num_shares_at_level(*bestBid, Side::BUY) << '\n';
    } else {
        out << "Best Bid: none\n";
    }

    if (bestAsk.has_value()) {
        out << "Best Ask: " << static_cast<double>(*bestAsk) / 10'000 << " x "
            << ob_.get_num_shares_at_level(*bestAsk, Side::SELL) << '\n';
    } else {
        out << "Best Ask: none\n";
    }

    out << "\nTop " << depth << " ask levels:\n";
    for (const auto& [price, volume] : ob_.get_top_ask_levels(depth)) {
        out << static_cast<double>(price) / 10'000 << " x " << volume << '\n';
    }

    out << "\nTop " << depth << " bid levels:\n";
    for (const auto& [price, volume] : ob_.get_top_bid_levels(depth)) {
        out << static_cast<double>(price) / 10'000 << " x " << volume << '\n';
    }
}

bool ItchReplayEngine::is_target_stock(data::StockLocate stockLocate) const {
    return targetStockLocate_.has_value() && (stockLocate == *targetStockLocate_);
}

bool ItchReplayEngine::matches_symbol(const std::array<char, 8>& itchStock,
                                      const std::string& symbol) {
    std::size_t itchStockSize = 0;

    while (itchStockSize < itchStock.size() && itchStock[itchStockSize] != ' ') {
        itchStockSize++;
    }

    return (itchStockSize == symbol.size()) &&
           std::equal(symbol.begin(), symbol.end(), itchStock.begin());
}

} // namespace lob::replay::engine