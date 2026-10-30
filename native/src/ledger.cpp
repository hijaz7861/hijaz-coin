#include "hijaz/ledger.hpp"

#include <limits>
#include <utility>

namespace hijaz {

ApplyResult Ledger::apply(
    LedgerState& state,
    const Transaction& tx,
    const SignatureVerifier& verifier) const {

    if (!tx.structurally_valid()) return ApplyResult::InvalidStructure;
    if (tx.chain_id != chain_id_) return ApplyResult::WrongChain;
    if (derive_account_address(tx.authorization_suite, tx.public_key) != tx.sender)
        return ApplyResult::SenderKeyMismatch;
    if (!verifier.supports(tx.authorization_suite))
        return ApplyResult::UnsupportedAuthorizationSuite;

    const auto sender_it = state.accounts.find(tx.sender);
    if (sender_it == state.accounts.end()) return ApplyResult::UnknownSender;

    const auto& sender = sender_it->second;
    if (tx.nonce != sender.next_nonce) return ApplyResult::BadNonce;

    if (tx.amount > std::numeric_limits<std::uint64_t>::max() - tx.fee)
        return ApplyResult::ArithmeticOverflow;
    const auto debit = tx.amount + tx.fee;
    if (sender.balance < debit) return ApplyResult::InsufficientFunds;

    if (tx.fee > std::numeric_limits<std::uint64_t>::max() - state.accumulated_fees)
        return ApplyResult::ArithmeticOverflow;
    if (sender.next_nonce == std::numeric_limits<std::uint64_t>::max())
        return ApplyResult::ArithmeticOverflow;

    const auto payload = tx.signing_payload();
    if (!verifier.verify(tx.authorization_suite, tx.public_key, payload, tx.signature))
        return ApplyResult::InvalidSignature;

    LedgerState candidate = state;
    auto& candidate_sender = candidate.accounts.at(tx.sender);
    auto& recipient = candidate.accounts[tx.receiver];

    if (tx.amount > std::numeric_limits<std::uint64_t>::max() - recipient.balance)
        return ApplyResult::ArithmeticOverflow;

    candidate_sender.balance -= debit;
    ++candidate_sender.next_nonce;
    recipient.balance += tx.amount;
    candidate.accumulated_fees += tx.fee;
    state = std::move(candidate);
    return ApplyResult::Applied;
}

} // namespace hijaz
