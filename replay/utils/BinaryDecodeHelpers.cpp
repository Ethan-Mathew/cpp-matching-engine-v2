#include "BinaryDecodeHelpers.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace lob::replay::utils {

std::uint16_t get_message_length(std::ifstream& file) {
    std::byte messageLengthBuffer[2]{};

    file.read(reinterpret_cast<char*>(messageLengthBuffer), 2);

    std::uint16_t messageLength = static_cast<int>(messageLengthBuffer[0]) << 8;
    messageLength |= static_cast<int>(messageLengthBuffer[1]);

    return messageLength;
}

char get_message_type(std::ifstream& file) {
    std::byte messageTypeBuffer[1]{};

    file.read(reinterpret_cast<char*>(messageTypeBuffer), 1);

    return static_cast<char>(messageTypeBuffer[0]);
}

std::uint16_t read_u16_be(std::ifstream& file) {
    std::byte buffer[2]{};
    file.read(reinterpret_cast<char*>(buffer), 2);

    return (static_cast<std::uint16_t>(buffer[0]) << 8) | static_cast<std::uint16_t>(buffer[1]);
}

std::uint32_t read_u32_be(std::ifstream& file) {
    std::byte buffer[4]{};
    file.read(reinterpret_cast<char*>(buffer), 4);

    std::uint32_t value = 0;

    for (std::size_t i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(buffer[i]) << (24 - (8 * i));
    }

    return value;
}

std::uint64_t read_u48_be(std::ifstream& file) {
    std::byte buffer[6]{};
    file.read(reinterpret_cast<char*>(buffer), 6);

    std::uint64_t value = 0;

    for (std::size_t i = 0; i < 6; ++i) {
        value |= static_cast<std::uint64_t>(buffer[i]) << (40 - (8 * i));
    }

    return value;
}

std::uint64_t read_u64_be(std::ifstream& file) {
    std::byte buffer[8]{};
    file.read(reinterpret_cast<char*>(buffer), 8);

    std::uint64_t value = 0;

    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(buffer[i]) << (56 - (8 * i));
    }

    return value;
}

char read_char(std::ifstream& file) {
    std::byte buffer[1]{};
    file.read(reinterpret_cast<char*>(buffer), 1);

    return static_cast<char>(buffer[0]);
}

std::string read_alpha(std::ifstream& file, std::size_t length) {
    std::string value(length, '\0');
    file.read(value.data(), static_cast<std::streamsize>(length));
    return value;
}

bool unexpected_message_length(std::uint16_t messageLength, char messageType) {
    switch (messageType) {
    case 'S':
        return messageLength != 12;
    case 'R':
        return messageLength != 39;
    case 'A':
        return messageLength != 36;
    case 'F':
        return messageLength != 40;
    case 'E':
        return messageLength != 31;
    case 'C':
        return messageLength != 36;
    case 'X':
        return messageLength != 23;
    case 'D':
        return messageLength != 19;
    case 'U':
        return messageLength != 35;
    default:
        return false;
    }
}

} // namespace lob::replay::utils