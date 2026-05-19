#pragma once

#include "ItchReplayEngine.hpp"

#include <fstream>
#include <string>
#include <optional>

namespace lob::replay::engine
{

class ItchParser
{
public:
    ItchParser() = delete;
    explicit ItchParser(const std::string& filePath);

    ItchParser(const ItchParser&) = delete;
    ItchParser& operator=(const ItchParser&) = delete;

    void parse(
        ItchReplayEngine& replayEngine, 
        std::optional<data::Timestamp> stopTimestamp = std::nullopt
    );

private:
    template <typename Message>
    bool should_stop_before(
        const Message& message,
        const std::optional<lob::replay::data::Timestamp>& stopTimestamp
    );

    std::ifstream file_;
};

template <typename Message>
bool ItchParser::should_stop_before(
    const Message& message,
    const std::optional<lob::replay::data::Timestamp>& stopTimestamp
)
{
    return stopTimestamp.has_value() &&
           message.timestamp_ > *stopTimestamp;
}

} // lob::replay::engine