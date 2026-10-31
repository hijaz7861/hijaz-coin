#pragma once

#include "hijaz/block_validation.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace hijaz {

inline constexpr std::size_t max_block_transactions = 4096U;
inline constexpr std::size_t max_block_wire_bytes = 16U * 1024U * 1024U;

enum class BlockDecodeError {
    None,
    TooShort,
    InvalidDomain,
    UnsupportedVersion,
    InvalidLength,
    TooManyTransactions,
    InvalidTransaction,
    NonCanonical
};

// Canonical block container: HJZB + container version + fixed V2 header +
// u32 transaction count + repeated (u32 byte length, canonical Transaction V3).
[[nodiscard]] std::vector<std::uint8_t> encode_block(const Block& block);
[[nodiscard]] BlockDecodeError decode_block(
    std::span<const std::uint8_t> bytes, Block& out);

} // namespace hijaz
