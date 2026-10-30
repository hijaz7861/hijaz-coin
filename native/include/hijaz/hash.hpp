#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace hijaz {
using Hash256 = std::array<std::uint8_t, 32>;

Hash256 sha3_256(std::span<const std::uint8_t> input);
Hash256 domain_hash(std::string_view domain, std::span<const std::uint8_t> input);
std::string to_hex(const Hash256& value);
} // namespace hijaz
