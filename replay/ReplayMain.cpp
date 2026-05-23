#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "data/CommonFieldAliases.hpp"
#include "engine/ItchParser.hpp"
#include "engine/ItchReplayEngine.hpp"
#include "lob/OrderBookConfig.hpp"

lob::replay::data::Timestamp parse_stop_time(const std::string& stopTime) {
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    char firstColon = '\0';
    char secondColon = '\0';

    std::istringstream input{stopTime};

    if (!(input >> hours >> firstColon >> minutes >> secondColon >> seconds) || firstColon != ':' ||
        secondColon != ':') {
        throw std::invalid_argument{"Invalid --stop-at time. Expected HH:MM:SS."};
    }

    input >> std::ws;

    if (!input.eof()) {
        throw std::invalid_argument{"Invalid --stop-at time. Expected HH:MM:SS."};
    }

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59) {
        throw std::invalid_argument{"Invalid --stop-at time value."};
    }

    constexpr lob::replay::data::Timestamp nanosecondsPerSecond = 1'000'000'000ULL;

    const lob::replay::data::Timestamp totalSeconds =
        static_cast<lob::replay::data::Timestamp>(hours) * 3600ULL +
        static_cast<lob::replay::data::Timestamp>(minutes) * 60ULL +
        static_cast<lob::replay::data::Timestamp>(seconds);

    return totalSeconds * nanosecondsPerSecond;
}

int main(int argc, char** argv) {
    if (argc != 6 && argc != 8) {
        std::cerr << "Usage:\n"
                  << "  ItchReplay <itch_file> <symbol> <initial_pool_size> "
                  << "<min_price> <max_price> [--stop-at HH:MM:SS]\n\n"
                  << "Examples:\n"
                  << "  ItchReplay data/full/03272019.NASDAQ_ITCH50.bin "
                  << "AAPL 2000000 1 10000000\n\n"
                  << "  ItchReplay data/full/03272019.NASDAQ_ITCH50.bin "
                  << "AAPL 2000000 1 10000000 --stop-at 10:00:00\n";

        return EXIT_FAILURE;
    }

    try {
        const std::string filePath = argv[1];
        const std::string symbol = argv[2];

        const std::size_t initialPoolSize = static_cast<std::size_t>(std::stoull(argv[3]));
        const lob::Price minPrice = static_cast<lob::Price>(std::stoll(argv[4]));
        const lob::Price maxPrice = static_cast<lob::Price>(std::stoll(argv[5]));
        std::optional<lob::replay::data::Timestamp> stopTimestamp = std::nullopt;
        std::optional<std::string> stopTimeLabel = std::nullopt;

        if (argc == 8) {
            const std::string option = argv[6];

            if (option != "--stop-at") {
                throw std::invalid_argument{"Unknown option. Expected --stop-at."};
            }

            stopTimeLabel = argv[7];
            stopTimestamp = parse_stop_time(*stopTimeLabel);
        }

        const lob::OrderBookConfig obConfig{initialPoolSize, minPrice, maxPrice};

        lob::replay::engine::ItchReplayEngine replayEngine{symbol, obConfig};
        lob::replay::engine::ItchParser parser{filePath};

        parser.parse(replayEngine, stopTimestamp);

        if (stopTimeLabel.has_value()) {
            std::cout << "Replay cutoff: " << *stopTimeLabel << "\n";
        }

        replayEngine.print_summary(std::cout);

        std::cout << "Replay completed successfully.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Replay failed: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}