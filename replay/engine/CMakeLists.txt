add_library(ReplayEngine
    ItchParser.cpp
    ItchReplayEngine.cpp
)

target_include_directories(ReplayEngine
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include    
        ${PROJECT_SOURCE_DIR}/replay
)

target_link_libraries(ReplayEngine
    PRIVATE
        OrderBook
        ReplayUtils
)

target_compile_features(ReplayEngine PUBLIC cxx_std_20)