#include "hijaz/block.hpp"
#include "hijaz/hash.hpp"
#include "hijaz/ledger.hpp"
#include "hijaz/transaction.hpp"

#include <cassert>
#include <iostream>

namespace {
class AcceptForUnitTests final : public hijaz::SignatureVerifier {
public:
    bool verify(const hijaz::Hash256&, const std::vector<std::uint8_t>&,
                const std::vector<std::uint8_t>& signature) const override {
        return signature == std::vector<std::uint8_t>{0xA5};
    }
};
}

int main() {
    using namespace hijaz;

    const std::vector<std::uint8_t> empty;
    assert(to_hex(sha3_256(empty)) ==
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");

    Transaction tx;
    tx.chain_id = 7;
    tx.sender[0] = 1;
    tx.receiver[0] = 2;
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
    Hash256 alice{};
    Hash256 bob{};
    alice[0] = 1;
    bob[0] = 2;
    state.accounts[alice] = AccountState{1000, 1};

    Transaction payment;
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

    std::cout << "HJZ native foundation and ledger tests: PASS\n";
}
