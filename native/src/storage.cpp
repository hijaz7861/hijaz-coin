#include "hijaz/storage.hpp"

#include <set>
#include <string>

namespace hijaz {

StoreCommitValidationError validate_atomic_block_commit(
    const AtomicBlockCommit& commit) {
    if (commit.block.header.version != 2)
        return StoreCommitValidationError::UnsupportedBlockVersion;
    if (commit.block.header.previous_hash != commit.expected_parent_hash)
        return StoreCommitValidationError::ParentMismatch;

    std::set<std::string> transaction_ids;
    for (const auto& transaction : commit.block.transactions) {
        if (!transaction.structurally_valid())
            return StoreCommitValidationError::InvalidTransaction;
        if (!transaction_ids.insert(to_hex(transaction.id())).second)
            return StoreCommitValidationError::InvalidTransaction;
    }

    if (merkle_root(commit.block.transactions) != commit.block.header.merkle_root)
        return StoreCommitValidationError::MerkleRootMismatch;
    if (ledger_state_root(commit.resulting_state) != commit.block.header.state_root)
        return StoreCommitValidationError::StateRootMismatch;
    return StoreCommitValidationError::None;
}

} // namespace hijaz
