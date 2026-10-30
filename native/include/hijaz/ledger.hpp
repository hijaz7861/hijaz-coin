#pragma once

#include "hijaz/transaction.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace hijaz {

struct AccountState {
    std::uint64_t balance{0};
    std::uint64_t next_nonce{1};
    bool operator==(const AccountState&) const = default;
};

struct LedgerState {
    std::map<Hash256, AccountState> accounts;
    std::uint64_t accumulated_fees{0};
};

// Deterministic commitment to all account balances, nonces, and accumulated fees.
// This is a prototype commitment format, not yet a production consensus rule.
[[nodiscard]] Hash256 ledger_state_root(const LedgerState& state);

class SignatureVerifier {
public:
    virtual ~SignatureVerifier() = default;
    [[nodiscard]] virtual bool supports(std::uint16_t authorization_suite) const noexcept = 0;
    [[nodiscard]] virtual bool verify(
        std::uint16_t authorization_suite,
        const std::vector<std::uint8_t>& public_key,
        const std::vector<std::uint8_t>& message,
        const std::vector<std::uint8_t>& signature) const = 0;
};

enum class ApplyResult {
    Applied,
    InvalidStructure,
    WrongChain,
    SenderKeyMismatch,
    UnknownSender,
    BadNonce,
    InsufficientFunds,
    ArithmeticOverflow,
    UnsupportedAuthorizationSuite,
    InvalidSignature
};

class Ledger {
public:
    explicit Ledger(std::uint32_t chain_id) : chain_id_(chain_id) {}

    [[nodiscard]] ApplyResult apply(
        LedgerState& state,
        const Transaction& tx,
        const SignatureVerifier& verifier) const;

private:
    std::uint32_t chain_id_;
};

} // namespace hijaz
