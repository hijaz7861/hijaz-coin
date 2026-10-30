#include "hijaz/block_validation.hpp"

#include <limits>
#include <set>
#include <string>

namespace hijaz {

BlockStructureError validate_block_structure(
    const Block& block,
    std::uint64_t expected_height,
    const Hash256& expected_parent,
    std::uint64_t parent_timestamp,
    std::uint64_t adjusted_network_time,
    const BlockValidationLimits& limits) {

    if (block.header.height != expected_height)
        return BlockStructureError::WrongHeight;
    if (block.header.previous_hash != expected_parent)
        return BlockStructureError::WrongParent;
    const auto latest_allowed = adjusted_network_time > std::numeric_limits<std::uint64_t>::max() - limits.max_future_drift_seconds
        ? UINT64_MAX : adjusted_network_time + limits.max_future_drift_seconds;
    if (block.header.timestamp <= parent_timestamp ||
        block.header.timestamp > latest_allowed)
        return BlockStructureError::InvalidTimestamp;
    if (block.transactions.size() > limits.max_transactions)
        return BlockStructureError::TooManyTransactions;

    std::set<std::string> seen;
    for (const auto& tx : block.transactions) {
        if (!tx.structurally_valid()) return BlockStructureError::InvalidTransaction;
        const auto id = to_hex(tx.id());
        if (!seen.insert(id).second) return BlockStructureError::DuplicateTransaction;
    }
    if (merkle_root(block.transactions) != block.header.merkle_root)
        return BlockStructureError::MerkleRootMismatch;
    return BlockStructureError::None;
}

} // namespace hijaz
