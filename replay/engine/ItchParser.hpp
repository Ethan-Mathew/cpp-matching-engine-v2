#pragma once

#include "ItchReplayEngine.hpp"

#include <fstream>
#include <string>

namespace lob::replay::engine
{

class ItchParser
{
public:
    ItchParser() = delete;
    explicit ItchParser(const std::string& filePath);

    ItchParser(const ItchParser&) = delete;
    ItchParser& operator=(const ItchParser&) = delete;

    void parse(ItchReplayEngine& replayEngine);

private:
    std::ifstream file_;
};

} // lob::replay::engine