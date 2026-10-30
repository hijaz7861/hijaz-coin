#pragma once

#include "hijaz/block_validation.hpp"
#include "hijaz/ledger.hpp"

#include <cstddef>

namespace hijaz {

enum class BlockExecutionError {
    None,
    TransactionRejected,
    StateRootMismatch
};

struct BlockExecutionResult {
    BlockExecutionError error{BlockExecutionError::None};
    std::size_t failed_transaction_index{static_cast<std::size_t>(-1)};
    ApplyResult transaction_result{ApplyResult::Applied};
};

// Executes transactions in block order against a candidate state. The caller
// must perform structural block validation separately before invoking this.
[[nodiscard]] BlockExecutionResult execute_block_transactions(
    const Block& block,
    LedgerState& state,
    const Ledger& ledger,
    const SignatureVerifier& verifier);

} // namespace hijaz
