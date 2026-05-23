#include "ItchParser.hpp"

#include <fstream>
#include <optional>
#include <stdexcept>

#include "ItchReplayEngine.hpp"
#include "data/CommonFieldAliases.hpp"
#include "data/DecodedMessageTypes.hpp"
#include "utils/BinaryDecodeHelpers.hpp"
#include "utils/BinaryMessageDecoders.hpp"

namespace lob::replay::engine {

ItchParser::ItchParser(const std::string& filePath) : file_{filePath, std::ios::binary} {
    if (!file_.is_open()) {
        throw std::invalid_argument{"File could not be opened."};
    }
}

void ItchParser::parse(ItchReplayEngine& replayEngine,
                       std::optional<data::Timestamp> stopTimestamp) {
    while (true) {
        const std::uint16_t messageLength = utils::get_message_length(file_);

        if (!file_) {
            break;
        }

        if (messageLength == 0) {
            break;
        }

        const char messageType = utils::get_message_type(file_);

        if (!file_) {
            break;
        }

        if (utils::unexpected_message_length(messageLength, messageType)) {
            throw std::runtime_error{"Unexpected ITCH message length."};
        }

        switch (messageType) {
        case 'S': {
            const data::SystemEventMessage message = utils::decode_s_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'R': {
            const data::StockDirectoryMessage message = utils::decode_r_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'A': {
            const data::AddOrderMessage message = utils::decode_a_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'F': {
            const data::AddOrderWithMPIDMessage message = utils::decode_f_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'E': {
            const data::OrderExecutedMessage message = utils::decode_e_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'C': {
            const data::OrderExecutedWithPriceMessage message = utils::decode_c_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'X': {
            const data::OrderCancelMessage message = utils::decode_x_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'D': {
            const data::OrderDeleteMessage message = utils::decode_d_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        case 'U': {
            const data::OrderReplaceMessage message = utils::decode_u_type_message(file_);

            if (should_stop_before(message, stopTimestamp)) {
                return;
            }

            replayEngine.on_message(message);

            break;
        }
        default:
            file_.ignore(static_cast<std::streamsize>(messageLength - 1));
            break;
        }

        if (!file_) {
            break;
        }
    }
}

} // namespace lob::replay::engine