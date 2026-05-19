#include "lob/OrderBookConfig.hpp"

#include "engine/ItchParser.hpp"
#include "engine/ItchReplayEngine.hpp"

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cerr << "Usage:\n"
                  << "  ItchReplay <itch_file> <symbol> <initial_pool_size> <min_price> <max_price>\n\n"
                  << "Example:\n"
                  << "  ItchReplay data/dev/itch_first_100mb.bin AAPL 1000000 100000 5000000\n";

        return EXIT_FAILURE;
    }

    try
    {
        const std::string filePath = argv[1];
        const std::string symbol = argv[2];

        const std::size_t initialPoolSize = static_cast<std::size_t>(std::stoull(argv[3]));
        const lob::Price minPrice = static_cast<lob::Price>(std::stoll(argv[4]));
        const lob::Price maxPrice = static_cast<lob::Price>(std::stoll(argv[5]));

        const lob::OrderBookConfig obConfig{initialPoolSize, minPrice, maxPrice};

        lob::replay::engine::ItchReplayEngine replayEngine{symbol, obConfig};
        lob::replay::engine::ItchParser parser{filePath};

        parser.parse(replayEngine);
        replayEngine.print_summary(std::cout);

        std::cout << "Replay completed successfully.\n";
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Replay failed: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}