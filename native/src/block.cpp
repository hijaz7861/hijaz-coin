#include "hijaz/block.hpp"

#include <span>
#include <utility>
#include <vector>

namespace hijaz {
namespace {
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}
void append_hash(std::vector<std::uint8_t>& out, const Hash256& hash) {
    out.insert(out.end(), hash.begin(), hash.end());
}
} // namespace

std::vector<std::uint8_t> BlockHeader::canonical_bytes() const {
    std::vector<std::uint8_t> out;
    out.reserve(4 + 8 + 8 + 32 + 32 + 8);
    append_u32(out, version);
    append_u64(out, height);
    append_u64(out, timestamp);
    append_hash(out, previous_hash);
    append_hash(out, merkle_root);
    append_u64(out, nonce);
    return out;
}

Hash256 BlockHeader::hash() const {
    const auto bytes = canonical_bytes();
    return domain_hash("HJZ/BLOCK/V1", bytes);
}

Hash256 merkle_root(const std::vector<Transaction>& transactions) {
    if (transactions.empty()) return domain_hash("HJZ/MERKLE/EMPTY/V1", {});
    std::vector<Hash256> level;
    level.reserve(transactions.size());
    for (const auto& tx : transactions) {
        const auto id = tx.id();
        level.push_back(domain_hash("HJZ/MERKLE/LEAF/V1", id));
    }
    while (level.size() > 1) {
        std::vector<Hash256> next;
        next.reserve((level.size() + 1) / 2);
        for (std::size_t i = 0; i < level.size(); i += 2) {
            const auto& right = (i + 1 < level.size()) ? level[i + 1] : level[i];
            std::vector<std::uint8_t> pair;
            pair.reserve(64);
            pair.insert(pair.end(), level[i].begin(), level[i].end());
            pair.insert(pair.end(), right.begin(), right.end());
            next.push_back(domain_hash("HJZ/MERKLE/NODE/V1", pair));
        }
        level = std::move(next);
    }
    return level.front();
}
} // namespace hijaz
