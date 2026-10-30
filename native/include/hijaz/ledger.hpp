#pragma once

#include "hijaz/transaction.hpp"

#include <cstdint>
#include <map>

namespace hijaz {

struct AccountState {
    std::uint64_t balance{0};
    std::uint64_t next_nonce{1};
};

struct LedgerState {
    std::map<Hash256, AccountState> accounts;
    std::uint64_t accumulated_fees{0};
};

class SignatureVerifier {
public:
    virtual ~SignatureVerifier() = default;
    [[nodiscard]] virtual bool verify(
        const Hash256& public_key,
        const std::vector<std::uint8_t>& message,
        const std::vector<std::uint8_t>& signature) const = 0;
};

enum class ApplyResult {
    Applied,
    InvalidStructure,
    WrongChain,
    UnknownSender,
    BadNonce,
    InsufficientFunds,
    ArithmeticOverflow,
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
