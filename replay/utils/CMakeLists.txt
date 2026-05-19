add_library(ReplayUtils
    BinaryDecodeHelpers.cpp
    BinaryMessageDecoders.cpp
)

target_include_directories(ReplayUtils
    PUBLIC
        ${PROJECT_SOURCE_DIR}/replay
)

target_compile_features(ReplayUtils PUBLIC cxx_std_20)