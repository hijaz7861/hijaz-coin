#pragma once

#include "hijaz/block.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hijaz {

struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;
};

enum class BlockStructureError {
    None,
    WrongHeight,
    WrongParent,
    InvalidTimestamp,
    TooManyTransactions,
    InvalidTransaction,
    DuplicateTransaction,
    MerkleRootMismatch
};

struct BlockValidationLimits {
    std::size_t max_transactions{4096};
    std::uint64_t max_future_drift_seconds{120};
};

[[nodiscard]] BlockStructureError validate_block_structure(
    const Block& block,
    std::uint64_t expected_height,
    const Hash256& expected_parent,
    std::uint64_t parent_timestamp,
    std::uint64_t adjusted_network_time,
    const BlockValidationLimits& limits = {});

} // namespace hijaz
