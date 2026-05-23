#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace lob::replay::utils {

std::uint16_t get_message_length(std::ifstream& file);
char get_message_type(std::ifstream& file);

std::uint16_t read_u16_be(std::ifstream& file);
std::uint32_t read_u32_be(std::ifstream& file);
std::uint64_t read_u48_be(std::ifstream& file);
std::uint64_t read_u64_be(std::ifstream& file);
char read_char(std::ifstream& file);
std::string read_alpha(std::ifstream& file, std::size_t length);
template <std::size_t N> inline std::array<char, N> read_alpha_array(std::ifstream& file) {
    std::array<char, N> value{};
    file.read(value.data(), static_cast<std::streamsize>(N));
    return value;
}

bool unexpected_message_length(std::uint16_t messageLength, char messageType);

} // namespace lob::replay::utils