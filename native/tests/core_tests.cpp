#include "hijaz/block.hpp"
#include "hijaz/block_execution.hpp"
#include "hijaz/block_validation.hpp"
#include "hijaz/hash.hpp"
#include "hijaz/ledger.hpp"
#ifdef HJZ_HAS_OPENSSL_ML_DSA
#include "hijaz/openssl_ml_dsa.hpp"
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <memory>
#endif
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

#ifdef HJZ_HAS_OPENSSL_ML_DSA
namespace {
using TestPkey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using TestPkeyContext = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
using TestSignature = std::unique_ptr<EVP_SIGNATURE, decltype(&EVP_SIGNATURE_free)>;

struct GeneratedMlDsaKey {
    TestPkey key{nullptr, EVP_PKEY_free};
    std::vector<std::uint8_t> public_key;
};

GeneratedMlDsaKey generate_ml_dsa65_key() {
    GeneratedMlDsaKey generated;
    generated.key.reset(EVP_PKEY_Q_keygen(nullptr, nullptr, "ML-DSA-65"));
    assert(generated.key != nullptr);

    std::size_t public_key_size = 0;
    assert(EVP_PKEY_get_octet_string_param(
        generated.key.get(), OSSL_PKEY_PARAM_PUB_KEY,
        nullptr, 0, &public_key_size) == 1);
    generated.public_key.resize(public_key_size);
    assert(EVP_PKEY_get_octet_string_param(
        generated.key.get(), OSSL_PKEY_PARAM_PUB_KEY,
        generated.public_key.data(), generated.public_key.size(),
        &public_key_size) == 1);
    generated.public_key.resize(public_key_size);
    return generated;
}

std::vector<std::uint8_t> sign_ml_dsa65(
    EVP_PKEY* key, const std::vector<std::uint8_t>& message) {
    TestSignature algorithm(EVP_SIGNATURE_fetch(nullptr, "ML-DSA-65", nullptr),
                            EVP_SIGNATURE_free);
    assert(algorithm != nullptr);
    TestPkeyContext context(EVP_PKEY_CTX_new_from_pkey(nullptr, key, nullptr),
                            EVP_PKEY_CTX_free);
    assert(context != nullptr);
    assert(EVP_PKEY_sign_message_init(context.get(), algorithm.get(), nullptr) == 1);

    std::size_t signature_size = 0;
    assert(EVP_PKEY_sign(context.get(), nullptr, &signature_size,
                         message.data(), message.size()) == 1);
    std::vector<std::uint8_t> signature(signature_size);
    assert(EVP_PKEY_sign(context.get(), signature.data(), &signature_size,
                         message.data(), message.size()) == 1);
    signature.resize(signature_size);
    return signature;
}
} // namespace
#endif

int main() {
    using namespace hijaz;

    const std::vector<std::uint8_t> empty;
    LedgerState empty_state;
    const auto empty_root = ledger_state_root(empty_state);
    assert(empty_root == ledger_state_root(empty_state));
    LedgerState ordered_state_a;
    Hash256 state_address_a{};
    Hash256 state_address_b{};
    state_address_a[0] = 0x01;
    state_address_b[0] = 0x02;
    ordered_state_a.accounts[state_address_b] = AccountState{200, 3};
    ordered_state_a.accounts[state_address_a] = AccountState{100, 2};
    LedgerState ordered_state_b;
    ordered_state_b.accounts[state_address_a] = AccountState{100, 2};
    ordered_state_b.accounts[state_address_b] = AccountState{200, 3};
    assert(ledger_state_root(ordered_state_a) == ledger_state_root(ordered_state_b));
    const auto committed_root = ledger_state_root(ordered_state_a);
    ordered_state_b.accounts[state_address_a].balance += 1;
    assert(ledger_state_root(ordered_state_b) != committed_root);
    ordered_state_b = ordered_state_a;
    ordered_state_b.accumulated_fees = 1;
    assert(ledger_state_root(ordered_state_b) != committed_root);
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
    payment.sender = derive_account_address(99, test_public_key);
    assert(ledger.apply(state, payment, test_verifier) == ApplyResult::UnsupportedAuthorizationSuite);
    assert(state.accounts == before_replay.accounts);

    payment.authorization_suite = 1;
    payment.sender = alice;
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
    candidate.header.version = 1;
    assert(validate_block_structure(candidate, 2, header.hash(), 100, 105) ==
           BlockStructureError::UnsupportedVersion);
    candidate.header.version = 2;
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

    // Block execution is atomic across the full transaction list.
    Hash256 charlie{};
    charlie[0] = 3;
    LedgerState block_state;
    block_state.accounts[alice] = AccountState{1000, 1};
    Block execution_block;
    Transaction first_payment;
    first_payment.authorization_suite = 1;
    first_payment.public_key = test_public_key;
    first_payment.chain_id = 7;
    first_payment.sender = alice;
    first_payment.receiver = bob;
    first_payment.amount = 50;
    first_payment.nonce = 1;
    first_payment.signature = {0xA5};
    Transaction second_payment = first_payment;
    second_payment.receiver = charlie;
    second_payment.amount = 60;
    second_payment.nonce = 2;
    execution_block.transactions = {first_payment, second_payment};
    LedgerState expected_block_state = block_state;
    assert(ledger.apply(expected_block_state, first_payment, test_verifier) == ApplyResult::Applied);
    assert(ledger.apply(expected_block_state, second_payment, test_verifier) == ApplyResult::Applied);
    execution_block.header.state_root = ledger_state_root(expected_block_state);

    const auto block_result = execute_block_transactions(
        execution_block, block_state, ledger, test_verifier);
    assert(block_result.error == BlockExecutionError::None);
    assert(block_state.accounts.at(alice).balance == 890);
    assert(block_state.accounts.at(alice).next_nonce == 3);
    assert(block_state.accounts.at(bob).balance == 50);
    assert(block_state.accounts.at(charlie).balance == 60);
    assert(ledger_state_root(block_state) == execution_block.header.state_root);

    LedgerState wrong_root_state;
    wrong_root_state.accounts[alice] = AccountState{1000, 1};
    const auto wrong_root_before = wrong_root_state;
    execution_block.header.state_root[0] ^= 0x01U;
    const auto wrong_root_result = execute_block_transactions(
        execution_block, wrong_root_state, ledger, test_verifier);
    assert(wrong_root_result.error == BlockExecutionError::StateRootMismatch);
    assert(wrong_root_state.accounts == wrong_root_before.accounts);
    execution_block.header.state_root = ledger_state_root(expected_block_state);

    LedgerState rejected_block_state;
    rejected_block_state.accounts[alice] = AccountState{1000, 1};
    second_payment.signature = {0x00};
    execution_block.transactions = {first_payment, second_payment};
    const auto rejected_before = rejected_block_state;
    const auto rejected_result = execute_block_transactions(
        execution_block, rejected_block_state, ledger, test_verifier);
    assert(rejected_result.error == BlockExecutionError::TransactionRejected);
    assert(rejected_result.failed_transaction_index == 1);
    assert(rejected_result.transaction_result == ApplyResult::InvalidSignature);
    assert(rejected_block_state.accounts == rejected_before.accounts);
    assert(rejected_block_state.accumulated_fees == rejected_before.accumulated_fees);

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

#ifdef HJZ_HAS_OPENSSL_ML_DSA
    // Real FIPS 204 ML-DSA-65 key generation, signing, import, and verification.
    // 0x7FFE is a test-only suite ID; it is not a production protocol assignment.
    constexpr std::uint16_t ml_dsa_test_suite = 0x7FFE;
    auto ml_dsa_key = generate_ml_dsa65_key();
    OpenSslMlDsaVerifier ml_dsa_verifier(ml_dsa_test_suite, "ML-DSA-65");

    Transaction pq_payment;
    pq_payment.authorization_suite = ml_dsa_test_suite;
    pq_payment.public_key = ml_dsa_key.public_key;
    pq_payment.chain_id = 77;
    pq_payment.sender = derive_account_address(ml_dsa_test_suite, ml_dsa_key.public_key);
    pq_payment.receiver = derive_account_address(
        ml_dsa_test_suite, std::vector<std::uint8_t>{0x44, 0x55});
    pq_payment.amount = 17;
    pq_payment.fee = 1;
    pq_payment.nonce = 1;
    pq_payment.signature = sign_ml_dsa65(
        ml_dsa_key.key.get(), pq_payment.signing_payload());

    Ledger pq_ledger(77);
    LedgerState pq_state;
    pq_state.accounts[pq_payment.sender] = AccountState{100, 1};
    assert(pq_ledger.apply(pq_state, pq_payment, ml_dsa_verifier) == ApplyResult::Applied);
    assert(pq_state.accounts.at(pq_payment.sender).balance == 82);
    assert(pq_state.accounts.at(pq_payment.receiver).balance == 17);

    const auto pq_before_invalid = pq_state;
    pq_payment.nonce = 2;
    pq_payment.signature[0] ^= 0x01U;
    assert(pq_ledger.apply(pq_state, pq_payment, ml_dsa_verifier) ==
           ApplyResult::InvalidSignature);
    assert(pq_state.accounts == pq_before_invalid.accounts);
#endif

    std::cout << "HJZ native foundation and ledger tests: PASS\n";
}
