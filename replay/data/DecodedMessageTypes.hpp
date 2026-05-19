#pragma once

#include "CommonFieldAliases.hpp"

#include <array>
#include <cstdint>

namespace lob::replay::data
{

// S - System Event Message
struct SystemEventMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    char eventCode_;
};

// R - Stock Directory Message
struct StockDirectoryMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    std::array<char, 8> stock_;
    char marketCategory_;
    char financialStatusIndicator_;
    std::uint32_t roundLotSize_;
    char roundLotsOnly_;
    char issueClassification_;
    std::array<char, 2> issueSubType_;
    char authenticity_;
    char shortSaleThresholdIndicator_;
    char ipoFlag_;
    char luldReferencePriceTier_;
    char etpFlag_;
    std::uint32_t etpLeverageFactor_;
    char inverseIndicator_;
};

// A - Add Order, no MPID Attribution
struct AddOrderMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
    char buySellIndicator_;
    Shares shares_;
    std::array<char, 8> stock_;
    Price price_;
};

// F - Add Order with MPID Attribution
struct AddOrderWithMPIDMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
    char buySellIndicator_;
    Shares shares_;
    std::array<char, 8> stock_;
    Price price_;
    std::array<char, 4> attribution_;
};

// E - Order Executed Message
struct OrderExecutedMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
    Shares executedShares_;
    MatchNumber matchNumber_;
};

// C - Order Executed With Price Message
struct OrderExecutedWithPriceMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
    Shares executedShares_;
    MatchNumber matchNumber_;
    char printable_;
    Price executionPrice_;
};

// X - Order Cancel Message
struct OrderCancelMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
    Shares cancelledShares_;
};

// D - Order Delete Message
struct OrderDeleteMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;
    OrderReferenceNumber orderReferenceNumber_;
};

// U - Order Replace Message
struct OrderReplaceMessage
{
    StockLocate stockLocate_;
    TrackingNumber trackingNumber_;
    Timestamp timestamp_;

    OrderReferenceNumber originalOrderReferenceNumber_;
    OrderReferenceNumber newOrderReferenceNumber_;
    Shares shares_;
    Price price_;
};

} // namespace lob::replay::data