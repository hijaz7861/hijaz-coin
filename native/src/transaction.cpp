#include "hijaz/transaction.hpp"

#include <algorithm>
#include <span>

namespace hijaz {
namespace {
void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>(value & 0xffU));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}
void append_bytes(std::vector<std::uint8_t>& out, std::span<const std::uint8_t> bytes) {
    out.insert(out.end(), bytes.begin(), bytes.end());
}
} // namespace

Hash256 derive_account_address(
    std::uint16_t authorization_suite,
    const std::vector<std::uint8_t>& canonical_public_key) {
    std::vector<std::uint8_t> input(2U + canonical_public_key.size());
    input[0] = static_cast<std::uint8_t>((authorization_suite >> 8U) & 0xffU);
    input[1] = static_cast<std::uint8_t>(authorization_suite & 0xffU);
    std::copy(canonical_public_key.begin(), canonical_public_key.end(), input.begin() + 2);
    return domain_hash("HJZ/ADDRESS/V1", input);
}

bool Transaction::structurally_valid() const noexcept {
    if (format_version != transaction_format_version || authorization_suite == 0)
        return false;
    if (public_key.empty() || public_key.size() > max_public_key_bytes)
        return false;
    if (signature.empty() || signature.size() > max_signature_bytes)
        return false;
    if (amount == 0 || sender == receiver) return false;
    return true;
}

std::vector<std::uint8_t> Transaction::signing_payload() const {
    std::vector<std::uint8_t> out;
    out.reserve(5U + 1U + 2U + 4U + 4U + public_key.size() +
                32U + 32U + 8U + 8U + 8U);
    constexpr std::uint8_t domain[] = {'H','J','Z','T',3};
    append_bytes(out, domain);
    out.push_back(format_version);
    append_u16(out, authorization_suite);
    append_u32(out, chain_id);
    append_u32(out, static_cast<std::uint32_t>(public_key.size()));
    append_bytes(out, public_key);
    append_bytes(out, sender);
    append_bytes(out, receiver);
    append_u64(out, amount);
    append_u64(out, fee);
    append_u64(out, nonce);
    return out;
}

std::vector<std::uint8_t> Transaction::canonical_bytes() const {
    auto out = signing_payload();
    append_u64(out, static_cast<std::uint64_t>(signature.size()));
    append_bytes(out, signature);
    return out;
}

Hash256 Transaction::id() const {
    const auto bytes = canonical_bytes();
    return domain_hash("HJZ/TXID/V3", bytes);
}
} // namespace hijaz
