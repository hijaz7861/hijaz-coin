#pragma once

#include "hijaz/block_validation.hpp"
#include "hijaz/ledger.hpp"

#include <cstdint>
#include <optional>

namespace hijaz {

struct StoredChainTip {
    std::uint64_t height{0};
    Hash256 block_hash{};
    Hash256 state_root{};
};

enum class StoreResult {
    Committed,
    TipConflict,
    MissingParent,
    CorruptData,
    StorageFailure
};

enum class StoreCommitValidationError {
    None,
    UnsupportedBlockVersion,
    ParentMismatch,
    InvalidTransaction,
    MerkleRootMismatch,
    StateRootMismatch
};

struct AtomicBlockCommit {
    Hash256 expected_parent_hash{};
    Block block;
    LedgerState resulting_state;
};

[[nodiscard]] StoreCommitValidationError validate_atomic_block_commit(
    const AtomicBlockCommit& commit);

class LedgerStore {
public:
    virtual ~LedgerStore() = default;

    [[nodiscard]] virtual std::optional<StoredChainTip> canonical_tip() const = 0;
    [[nodiscard]] virtual std::optional<Hash256> canonical_hash_at(
        std::uint64_t height) const = 0;
    [[nodiscard]] virtual std::optional<Block> block_by_hash(
        const Hash256& block_hash) const = 0;
    [[nodiscard]] virtual std::optional<LedgerState> load_state() const = 0;

    // Implementations MUST atomically publish block bytes, canonical height
    // index, resulting ledger state, transaction indexes, state root, tip,
    // and undo data. On any failure, none of those changes may become visible.
    // expected_parent_hash provides optimistic concurrency protection.
    // Implementations must call validate_atomic_block_commit before writing.
    [[nodiscard]] virtual StoreResult commit_validated_block_atomically(
        const AtomicBlockCommit& commit) = 0;
};

} // namespace hijaz
