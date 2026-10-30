#include "hijaz/block.hpp"
#include "hijaz/block_validation.hpp"
#include "hijaz/hash.hpp"
#include "hijaz/ledger.hpp"
#include "hijaz/p2p_frame.hpp"
#include "hijaz/transaction.hpp"

#include <cassert>
#include <iostream>
#include <limits>

namespace {
class AcceptForUnitTests final : public hijaz::SignatureVerifier {
public:
    bool supports(std::uint16_t suite) const noexcept override { return suite == 1; }

    bool verify(std::uint16_t suite, const std::vector<std::uint8_t>& public_key,
                const std::vector<std::uint8_t>&,
                const std::vector<std::uint8_t>& signature) const override {
        return suite == 1 && public_key == std::vector<std::uint8_t>{0x01, 0x02} &&
               signature == std::vector<std::uint8_t>{0xA5};
    }
};
}

int main() {
    using namespace hijaz;

    const std::vector<std::uint8_t> empty;
    assert(to_hex(sha3_256(empty)) ==
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");

    const std::vector<std::uint8_t> test_public_key{0x01, 0x02};
    const auto alice = derive_account_address(1, test_public_key);
    const auto bob = derive_account_address(1, std::vector<std::uint8_t>{0x03, 0x04});
    assert(alice != bob);
    assert(alice != derive_account_address(2, test_public_key));

    Transaction tx;
    tx.authorization_suite = 1;
    tx.public_key = test_public_key;
    tx.chain_id = 7;
    tx.sender = alice;
    tx.receiver = bob;
    tx.amount = 100;
    tx.fee = 3;
    tx.nonce = 7;
    tx.signature = {0x01, 0x02, 0x03};
    assert(tx.structurally_valid());

    const auto first_id = tx.id();
    tx.signature.push_back(0x04);
    assert(tx.id() != first_id);
    assert(merkle_root({tx}) != merkle_root({}));

    BlockHeader header;
    header.height = 1;
    header.merkle_root = merkle_root({tx});
    const auto first_block_hash = header.hash();
    ++header.nonce;
    assert(header.hash() != first_block_hash);

    tx.amount = 0;
    assert(!tx.structurally_valid());

    // Ledger tests use a deliberately fake verifier. This is not cryptography.
    AcceptForUnitTests test_verifier;
    Ledger ledger(7);
    LedgerState state;
    state.accounts[alice] = AccountState{1000, 1};

    Transaction payment;
    payment.authorization_suite = 1;
    payment.public_key = test_public_key;
    payment.chain_id = 7;
    payment.sender = alice;
    payment.receiver = bob;
    payment.amount = 125;
    payment.fee = 5;
    payment.nonce = 1;
    payment.signature = {0xA5};

    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::Applied);
    assert(state.accounts.at(alice).balance == 870);
    assert(state.accounts.at(alice).next_nonce == 2);
    assert(state.accounts.at(bob).balance == 125);
    assert(state.accumulated_fees == 5);

    const auto before_replay = state;
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::BadNonce);
    assert(state.accounts == before_replay.accounts);
    assert(state.accumulated_fees == before_replay.accumulated_fees);

    payment.nonce = 2;
    payment.chain_id = 8;
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::WrongChain);
    assert(state.accounts == before_replay.accounts);

    payment.chain_id = 7;
    payment.signature = {0x00};
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::InvalidSignature);
    assert(state.accounts == before_replay.accounts);

    payment.signature = {0xA5};
    payment.authorization_suite = 99;
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::UnsupportedAuthorizationSuite);
    assert(state.accounts == before_replay.accounts);

    payment.authorization_suite = 1;
    payment.sender[0] ^= 0x01U;
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::SenderKeyMismatch);
    assert(state.accounts == before_replay.accounts);
    payment.sender = alice;

    // A recipient overflow must not debit the sender or advance its nonce.
    LedgerState overflow_state;
    overflow_state.accounts[alice] = AccountState{100, 1};
    overflow_state.accounts[bob] = AccountState{std::numeric_limits<std::uint64_t>::max(), 1};
    Transaction overflow_payment;
    overflow_payment.authorization_suite = 1;
    overflow_payment.public_key = test_public_key;
    overflow_payment.chain_id = 7;
    overflow_payment.sender = alice;
    overflow_payment.receiver = bob;
    overflow_payment.amount = 1;
    overflow_payment.nonce = 1;
    overflow_payment.signature = {0xA5};
    const auto before_overflow = overflow_state;
    assert(ledger.apply(overflow_state, overflow_payment, test_verifier) ==
           ApplyResult::ArithmeticOverflow);
    assert(overflow_state.accounts == before_overflow.accounts);
    assert(overflow_state.accumulated_fees == before_overflow.accumulated_fees);

    Block candidate;
    candidate.header.height = 2;
    candidate.header.timestamp = 101;
    candidate.header.previous_hash = header.hash();
    candidate.transactions = {payment};
    candidate.header.merkle_root = merkle_root(candidate.transactions);
    assert(validate_block_structure(candidate, 2, header.hash(), 100, 105) ==
           BlockStructureError::None);
    candidate.header.height = 3;
    assert(validate_block_structure(candidate, 2, header.hash(), 100, 105) ==
           BlockStructureError::WrongHeight);
    candidate.header.height = 2;
    candidate.transactions = {payment, payment};
    candidate.header.merkle_root = merkle_root(candidate.transactions);
    assert(validate_block_structure(candidate, 2, header.hash(), 100, 105) ==
           BlockStructureError::DuplicateTransaction);
    candidate.transactions = {payment};
    candidate.header.merkle_root = merkle_root(candidate.transactions);
    candidate.header.timestamp = 100;
    assert(validate_block_structure(candidate, 2, header.hash(), 100, 105) ==
           BlockStructureError::InvalidTimestamp);

    const std::vector<std::uint8_t> ping_payload{0x10, 0x20, 0x30};
    const auto frame = p2p::encode_frame(p2p::MessageType::Ping, ping_payload);
    p2p::Frame decoded{p2p::MessageType::Reject, {}};
    assert(p2p::decode_frame(frame, decoded) == p2p::FrameError::None);
    assert(decoded.type == p2p::MessageType::Ping);
    assert(decoded.payload == ping_payload);
    auto corrupted = frame;
    corrupted[0] ^= 0xffU;
    assert(p2p::decode_frame(corrupted, decoded) == p2p::FrameError::BadMagic);
    auto truncated = frame;
    truncated.pop_back();
    assert(p2p::decode_frame(truncated, decoded) == p2p::FrameError::LengthMismatch);

    std::cout << "HJZ native foundation and ledger tests: PASS\n";
}
