#include "hijaz/ledger.hpp"

#include <limits>
#include <utility>
#include <vector>

namespace hijaz {
namespace {
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
}

Hash256 account_leaf_hash(const Hash256& address, const AccountState& account) {
    std::vector<std::uint8_t> encoded;
    encoded.reserve(48);
    encoded.insert(encoded.end(), address.begin(), address.end());
    append_u64(encoded, account.balance);
    append_u64(encoded, account.next_nonce);
    return domain_hash("HJZ/STATE/ACCOUNT/V1", encoded);
}

Hash256 merkle_accounts(std::vector<Hash256> level) {
    if (level.empty()) return domain_hash("HJZ/STATE/EMPTY/V1", {});
    while (level.size() > 1) {
        std::vector<Hash256> next;
        next.reserve((level.size() + 1U) / 2U);
        for (std::size_t i = 0; i < level.size(); i += 2U) {
            const auto& right = (i + 1U < level.size()) ? level[i + 1U] : level[i];
            std::vector<std::uint8_t> pair;
            pair.reserve(64);
            pair.insert(pair.end(), level[i].begin(), level[i].end());
            pair.insert(pair.end(), right.begin(), right.end());
            next.push_back(domain_hash("HJZ/STATE/NODE/V1", pair));
        }
        level = std::move(next);
    }
    return level.front();
}
} // namespace

Hash256 ledger_state_root(const LedgerState& state) {
    std::vector<Hash256> leaves;
    leaves.reserve(state.accounts.size());
    // std::map<Hash256, ...> supplies canonical lexicographic address ordering.
    for (const auto& [address, account] : state.accounts)
        leaves.push_back(account_leaf_hash(address, account));

    const auto accounts_root = merkle_accounts(std::move(leaves));
    std::vector<std::uint8_t> commitment;
    commitment.reserve(8U + 8U + 32U);
    append_u64(commitment, static_cast<std::uint64_t>(state.accounts.size()));
    append_u64(commitment, state.accumulated_fees);
    commitment.insert(commitment.end(), accounts_root.begin(), accounts_root.end());
    return domain_hash("HJZ/STATE/ROOT/V1", commitment);
}

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
