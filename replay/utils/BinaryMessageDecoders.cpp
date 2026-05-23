#include "BinaryMessageDecoders.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "BinaryDecodeHelpers.hpp"
#include "data/CommonFieldAliases.hpp"
#include "data/DecodedMessageTypes.hpp"

namespace lob::replay::utils {

data::SystemEventMessage decode_s_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const char eventCode = read_char(file);

    return data::SystemEventMessage{stockLocate, trackingNumber, timestamp, eventCode};
}

data::StockDirectoryMessage decode_r_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const std::array<char, 8> stock = read_alpha_array<8>(file);
    const char marketCategory = read_char(file);
    const char financialStatusIndicator = read_char(file);
    const std::uint32_t roundLotSize = read_u32_be(file);
    const char roundLotsOnly = read_char(file);
    const char issueClassification = read_char(file);
    const std::array<char, 2> issueSubType = read_alpha_array<2>(file);
    const char authenticity = read_char(file);
    const char shortSaleThresholdIndicator = read_char(file);
    const char ipoFlag = read_char(file);
    const char luldReferencePriceTier = read_char(file);
    const char etpFlag = read_char(file);
    const std::uint32_t etpLeverageFactor = read_u32_be(file);
    const char inverseIndicator = read_char(file);

    return data::StockDirectoryMessage{stockLocate,
                                       trackingNumber,
                                       timestamp,
                                       stock,
                                       marketCategory,
                                       financialStatusIndicator,
                                       roundLotSize,
                                       roundLotsOnly,
                                       issueClassification,
                                       issueSubType,
                                       authenticity,
                                       shortSaleThresholdIndicator,
                                       ipoFlag,
                                       luldReferencePriceTier,
                                       etpFlag,
                                       etpLeverageFactor,
                                       inverseIndicator};
}

data::AddOrderMessage decode_a_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);
    const char buySellIndicator = read_char(file);
    const data::Shares shares = read_u32_be(file);
    const std::array<char, 8> stock = read_alpha_array<8>(file);
    const data::Price price = read_u32_be(file);

    return data::AddOrderMessage{stockLocate,      trackingNumber, timestamp, orderReferenceNumber,
                                 buySellIndicator, shares,         stock,     price};
}

data::AddOrderWithMPIDMessage decode_f_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);
    const char buySellIndicator = read_char(file);
    const data::Shares shares = read_u32_be(file);
    const std::array<char, 8> stock = read_alpha_array<8>(file);
    const data::Price price = read_u32_be(file);
    const std::array<char, 4> attribution = read_alpha_array<4>(file);

    return data::AddOrderWithMPIDMessage{
        stockLocate, trackingNumber, timestamp, orderReferenceNumber, buySellIndicator,
        shares,      stock,          price,     attribution};
}

data::OrderExecutedMessage decode_e_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);
    const data::Shares executedShares = read_u32_be(file);
    const data::MatchNumber matchNumber = read_u64_be(file);

    return data::OrderExecutedMessage{stockLocate,          trackingNumber, timestamp,
                                      orderReferenceNumber, executedShares, matchNumber};
}

data::OrderExecutedWithPriceMessage decode_c_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);
    const data::Shares executedShares = read_u32_be(file);
    const data::MatchNumber matchNumber = read_u64_be(file);
    const char printable = read_char(file);
    const data::Price executionPrice = read_u32_be(file);

    return data::OrderExecutedWithPriceMessage{
        stockLocate,    trackingNumber, timestamp, orderReferenceNumber,
        executedShares, matchNumber,    printable, executionPrice};
}

data::OrderCancelMessage decode_x_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);
    const data::Shares cancelledShares = read_u32_be(file);

    return data::OrderCancelMessage{stockLocate, trackingNumber, timestamp, orderReferenceNumber,
                                    cancelledShares};
}

data::OrderDeleteMessage decode_d_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber orderReferenceNumber = read_u64_be(file);

    return data::OrderDeleteMessage{stockLocate, trackingNumber, timestamp, orderReferenceNumber};
}

data::OrderReplaceMessage decode_u_type_message(std::ifstream& file) {
    const data::StockLocate stockLocate = read_u16_be(file);
    const data::TrackingNumber trackingNumber = read_u16_be(file);
    const data::Timestamp timestamp = read_u48_be(file);
    const data::OrderReferenceNumber originalOrderReferenceNumber = read_u64_be(file);
    const data::OrderReferenceNumber newOrderReferenceNumber = read_u64_be(file);
    const data::Shares shares = read_u32_be(file);
    const data::Price price = read_u32_be(file);

    return data::OrderReplaceMessage{stockLocate,
                                     trackingNumber,
                                     timestamp,
                                     originalOrderReferenceNumber,
                                     newOrderReferenceNumber,
                                     shares,
                                     price};
}

} // namespace lob::replay::utils