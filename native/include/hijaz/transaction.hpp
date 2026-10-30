#pragma once

#include "hijaz/hash.hpp"

#include <cstdint>
#include <vector>

namespace hijaz {
struct Transaction {
    std::uint32_t chain_id{0};
    Hash256 sender{};
    Hash256 receiver{};
    std::uint64_t amount{0}; // smallest indivisible HJZ unit; never use floating point
    std::uint64_t fee{0};
    std::uint64_t nonce{0};
    std::vector<std::uint8_t> signature;

    [[nodiscard]] bool structurally_valid() const noexcept;
    [[nodiscard]] std::vector<std::uint8_t> signing_payload() const;
    [[nodiscard]] std::vector<std::uint8_t> canonical_bytes() const;
    [[nodiscard]] Hash256 id() const;
};
} // namespace hijaz
