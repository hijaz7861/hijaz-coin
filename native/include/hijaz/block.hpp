#pragma once

#include "hijaz/hash.hpp"
#include "hijaz/transaction.hpp"

#include <cstdint>
#include <vector>

namespace hijaz {
struct BlockHeader {
    std::uint32_t version{2};
    std::uint64_t height{0};
    std::uint64_t timestamp{0};
    Hash256 previous_hash{};
    Hash256 merkle_root{};
    Hash256 state_root{};
    std::uint64_t nonce{0};

    [[nodiscard]] std::vector<std::uint8_t> canonical_bytes() const;
    [[nodiscard]] Hash256 hash() const;
};

Hash256 merkle_root(const std::vector<Transaction>& transactions);
} // namespace hijaz
