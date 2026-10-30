#pragma once

#include "hijaz/hash.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hijaz {
inline constexpr std::uint8_t transaction_format_version = 3;
inline constexpr std::size_t max_public_key_bytes = 16U * 1024U;
inline constexpr std::size_t max_signature_bytes = 16U * 1024U;

// The suite identifier is protocol data. A zero/unknown suite is always rejected.
struct Transaction {
    std::uint8_t format_version{transaction_format_version};
    std::uint16_t authorization_suite{0};
    std::vector<std::uint8_t> public_key;
    std::uint32_t chain_id{0};
    Hash256 sender{};   // derived from authorization_suite + public_key
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

[[nodiscard]] Hash256 derive_account_address(
    std::uint16_t authorization_suite,
    const std::vector<std::uint8_t>& canonical_public_key);
} // namespace hijaz
