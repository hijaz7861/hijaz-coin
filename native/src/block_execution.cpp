#include "hijaz/block_execution.hpp"

#include <utility>

namespace hijaz {

BlockExecutionResult execute_block_transactions(
    const Block& block,
    LedgerState& state,
    const Ledger& ledger,
    const SignatureVerifier& verifier) {

    LedgerState candidate = state;
    for (std::size_t index = 0; index < block.transactions.size(); ++index) {
        const auto result = ledger.apply(candidate, block.transactions[index], verifier);
        if (result != ApplyResult::Applied) {
            return {
                BlockExecutionError::TransactionRejected,
                index,
                result
            };
        }
    }

    if (ledger_state_root(candidate) != block.header.state_root) {
        return {BlockExecutionError::StateRootMismatch, block.transactions.size(),
                ApplyResult::Applied};
    }

    state = std::move(candidate);
    return {};
}

} // namespace hijaz
