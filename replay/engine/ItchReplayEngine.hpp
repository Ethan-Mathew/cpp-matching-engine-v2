#pragma once

#include "lob/OrderBook.hpp"
#include "lob/OrderBookConfig.hpp"

#include "data/CommonFieldAliases.hpp"
#include "data/DecodedMessageTypes.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>

namespace lob::replay::engine
{

class ItchReplayEngine
{
public:
    ItchReplayEngine() = delete;
    explicit ItchReplayEngine(const std::string& symbol, const OrderBookConfig& obConfig);

    ItchReplayEngine(const ItchReplayEngine&) = delete;
    ItchReplayEngine& operator=(const ItchReplayEngine&) = delete;

    void on_message(const data::SystemEventMessage& message);
    void on_message(const data::StockDirectoryMessage& message);
    void on_message(const data::AddOrderMessage& message);
    void on_message(const data::AddOrderWithMPIDMessage& message);
    void on_message(const data::OrderExecutedMessage& message);
    void on_message(const data::OrderExecutedWithPriceMessage& message);
    void on_message(const data::OrderCancelMessage& message);
    void on_message(const data::OrderDeleteMessage& message);
    void on_message(const data::OrderReplaceMessage& message);
    
    void print_summary(std::ostream& out, std::size_t depth = 5) const;

private:
    bool is_target_stock(data::StockLocate stockLocate) const;
    bool matches_symbol(const std::array<char, 8>& itchStock, const std::string& symbol);

    std::size_t addsApplied_ = 0;
    std::size_t executionsApplied_ = 0;
    std::size_t cancelsApplied_ = 0;
    std::size_t deletesApplied_ = 0;
    std::size_t replacesApplied_ = 0;
    std::size_t failedApplications_ = 0;

    std::string symbol_;
    std::optional<data::StockLocate> targetStockLocate_;
    OrderBook ob_;
};

} // lob::replay::engine