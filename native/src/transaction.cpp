#include "hijaz/transaction.hpp"

#include <limits>

namespace hijaz {
namespace {
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}
void append_bytes(std::vector<std::uint8_t>& out, std::span<const std::uint8_t> bytes) {
    out.insert(out.end(), bytes.begin(), bytes.end());
}
} // namespace

bool Transaction::structurally_valid() const noexcept {
    if (amount == 0 || sender == receiver || signature.empty()) return false;
    return signature.size() <= 16U * 1024U;
}

std::vector<std::uint8_t> Transaction::signing_payload() const {
    std::vector<std::uint8_t> out;
    out.reserve(4 + 32 + 32 + 8 + 8);
    constexpr std::uint8_t domain[] = {'H','J','Z','T',1};
    append_bytes(out, domain);
    append_bytes(out, sender);
    append_bytes(out, receiver);
    append_u64(out, amount);
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
    return domain_hash("HJZ/TXID/V1", bytes);
}
} // namespace hijaz
