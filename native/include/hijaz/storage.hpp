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
    DuplicateTransaction,
    MerkleRootMismatch,
    StateRootMismatch
};

struct AtomicBlockCommit {
    Hash256 expected_parent_hash{};
    Block block;
    // Untrusted assertion from the caller. A durable store MUST replay the
    // block from its own persisted parent state and compare the internally
    // computed state with this value; it must never persist this value as
    // authoritative merely because its state root matches the header.
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

    // SECURITY CONTRACT: validate_atomic_block_commit is only a structural /
    // commitment preflight. It does not prove that resulting_state is the
    // result of executing this block against the store's actual parent state.
    // A durable implementation MUST, within a serialized database write
    // transaction, re-read the canonical tip and parent state, re-check the
    // expected parent and contiguous height, replay every transaction in order
    // with the configured production SignatureVerifier, verify the computed
    // state root, and treat resulting_state only as an assertion to compare.
    //
    // Implementations MUST atomically publish block bytes, canonical height
    // index, internally computed ledger state, transaction indexes, state
    // root, tip, and undo data. On any failure, none may become visible.
    // The expected-parent comparison MUST occur inside the same write
    // transaction as the commit to prevent two competing writers.
    [[nodiscard]] virtual StoreResult commit_validated_block_atomically(
        const AtomicBlockCommit& commit) = 0;
};

} // namespace hijaz
