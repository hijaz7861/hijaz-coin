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

namespace {
bool read_u8(std::span<const std::uint8_t> b, std::size_t& p, std::uint8_t& v) {
    if (p >= b.size()) return false;
    v = b[p++];
    return true;
}
bool read_u16(std::span<const std::uint8_t> b, std::size_t& p, std::uint16_t& v) {
    if (b.size() - p < 2U) return false;
    v = static_cast<std::uint16_t>((static_cast<std::uint16_t>(b[p]) << 8U) |
                                   static_cast<std::uint16_t>(b[p + 1U]));
    p += 2U;
    return true;
}
bool read_u32(std::span<const std::uint8_t> b, std::size_t& p, std::uint32_t& v) {
    if (b.size() - p < 4U) return false;
    v = (static_cast<std::uint32_t>(b[p]) << 24U) |
        (static_cast<std::uint32_t>(b[p + 1U]) << 16U) |
        (static_cast<std::uint32_t>(b[p + 2U]) << 8U) |
         static_cast<std::uint32_t>(b[p + 3U]);
    p += 4U;
    return true;
}
bool read_u64(std::span<const std::uint8_t> b, std::size_t& p, std::uint64_t& v) {
    if (b.size() - p < 8U) return false;
    v = 0;
    for (std::size_t i = 0; i < 8U; ++i)
        v = (v << 8U) | static_cast<std::uint64_t>(b[p + i]);
    p += 8U;
    return true;
}
bool take_bytes(std::span<const std::uint8_t> b, std::size_t& p,
                std::size_t n, std::span<const std::uint8_t>& value) {
    if (n > b.size() - p) return false;
    value = b.subspan(p, n);
    p += n;
    return true;
}
} // namespace

TransactionDecodeError decode_transaction(
    std::span<const std::uint8_t> bytes, Transaction& out) {
    constexpr std::size_t fixed_minimum =
        5U + 1U + 2U + 4U + 4U + 32U + 32U + 8U + 8U + 8U + 8U;
    if (bytes.size() < fixed_minimum) return TransactionDecodeError::TooShort;
    constexpr std::uint8_t domain[] = {'H','J','Z','T',3};
    for (std::size_t i = 0; i < sizeof(domain); ++i)
        if (bytes[i] != domain[i]) return TransactionDecodeError::InvalidDomain;

    std::size_t pos = sizeof(domain);
    Transaction candidate;
    if (!read_u8(bytes, pos, candidate.format_version))
        return TransactionDecodeError::TooShort;
    if (candidate.format_version != transaction_format_version)
        return TransactionDecodeError::UnsupportedVersion;
    if (!read_u16(bytes, pos, candidate.authorization_suite))
        return TransactionDecodeError::TooShort;
    if (candidate.authorization_suite == 0)
        return TransactionDecodeError::InvalidSuite;
    if (!read_u32(bytes, pos, candidate.chain_id))
        return TransactionDecodeError::TooShort;

    std::uint32_t public_key_length = 0;
    if (!read_u32(bytes, pos, public_key_length))
        return TransactionDecodeError::TooShort;
    if (public_key_length == 0 || public_key_length > max_public_key_bytes)
        return TransactionDecodeError::InvalidLength;
    std::span<const std::uint8_t> field;
    if (!take_bytes(bytes, pos, public_key_length, field))
        return TransactionDecodeError::TooShort;
    candidate.public_key.assign(field.begin(), field.end());

    if (!take_bytes(bytes, pos, candidate.sender.size(), field))
        return TransactionDecodeError::TooShort;
    std::copy(field.begin(), field.end(), candidate.sender.begin());
    if (!take_bytes(bytes, pos, candidate.receiver.size(), field))
        return TransactionDecodeError::TooShort;
    std::copy(field.begin(), field.end(), candidate.receiver.begin());
    if (!read_u64(bytes, pos, candidate.amount) ||
        !read_u64(bytes, pos, candidate.fee) ||
        !read_u64(bytes, pos, candidate.nonce))
        return TransactionDecodeError::TooShort;

    std::uint64_t signature_length = 0;
    if (!read_u64(bytes, pos, signature_length))
        return TransactionDecodeError::TooShort;
    if (signature_length == 0 || signature_length > max_signature_bytes)
        return TransactionDecodeError::InvalidLength;
    if (signature_length > static_cast<std::uint64_t>(bytes.size() - pos))
        return TransactionDecodeError::TooShort;
    if (signature_length != static_cast<std::uint64_t>(bytes.size() - pos))
        return TransactionDecodeError::NonCanonical;
    if (!take_bytes(bytes, pos, static_cast<std::size_t>(signature_length), field))
        return TransactionDecodeError::TooShort;
    candidate.signature.assign(field.begin(), field.end());
    if (pos != bytes.size()) return TransactionDecodeError::NonCanonical;
    if (!candidate.structurally_valid())
        return TransactionDecodeError::InvalidStructure;

    const auto canonical = candidate.canonical_bytes();
    if (canonical.size() != bytes.size() ||
        !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
        return TransactionDecodeError::NonCanonical;
    out = std::move(candidate);
    return TransactionDecodeError::None;
}

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
