# HJZ Native Layer-1 — Foundation

This directory contains the early C++20 implementation of an independent Layer-1. It is deliberately separate from the legacy ERC-20 prototype in `contracts/`; the ERC-20 contract is not the native HJZ coin.

## Current prototype scope

- Canonical block-header, complete bounded block-container encoding/decoding, and Transaction V3 serialization, plus a bounded canonical Transaction V3 decoder with strict field-length checks, exact framing, structural validation, canonical re-encoding, and output preservation on failure. A bounded canonical block-container encoder/decoder is implemented in `block_wire.hpp` / `block_wire.cpp`: it frames the V2 header and length-prefixed Transaction V3 records, caps the transaction count at 4096 and total encoded size at 16 MiB, rejects non-canonical/truncated input, and preserves output on decode failure. See ADR-0004.
- Transaction V3 carries an explicit authorization-suite ID and bounded public-key bytes.
- Account addresses are derived from suite ID + canonical public key using domain-separated SHA3-256; ledger rejects sender/key mismatches.
- Chain-ID binding, integer amount/fee fields, sequential account nonces, and deterministic signing payload.
- SHA3-256 through OpenSSL EVP.
- Domain-separated transaction IDs, block hashes, transaction Merkle roots, and deterministic ledger state roots over sorted account address/balance/nonce leaves plus accumulated fees.
- Block header V2 commits to the state root; unsupported header versions are rejected.
- Deterministic ledger transitions with balance, fee, nonce, replay, overflow, and atomic candidate-state checks.
- Block transaction executor applies transactions in canonical block order, checks the computed state root against the V2 header, and commits the resulting ledger state only if every transaction succeeds and the root matches.
- Explicit verifier capability check; unknown authorization suites are rejected.
- OpenSSL 3.5+ ML-DSA verifier for ML-DSA-44/65/87 with strict public-key and signature size checks.
- Conditional real ML-DSA-65 key-generation, signing, verification, and ledger-authorization test when OpenSSL 3.5+ is available.
- Structural block checks for supported header version, height, parent linkage, timestamp bounds, transaction count, duplicates, and transaction Merkle root.
- Vendor-neutral LedgerStore interface and pre-commit validator checking header version, parent hash, transaction structure/uniqueness, transaction Merkle root, and resulting state root. The validator is only a preflight: ADR-0011 requires a durable backend to replay transactions from its own persisted parent state using a production signature verifier; caller-supplied resulting state is untrusted.
- Bounded P2P frame codec with protocol magic/version, typed messages, length checks, and a 2 MiB payload cap; outbound encoding rejects unknown message types.
- P2P framing is transport scaffolding only: it currently has no frame checksum/MAC, peer authentication, encryption, replay protection, handshake state machine, or socket-level resource controls. Do not expose it to untrusted networks as a production peer protocol.
- Unit tests for hashing, deterministic state roots, Transaction V3 decoder round-trip and malformed-input rejection, address derivation, key/address mismatch, suite rejection, ledger success/replay/chain/signature failures, recipient-overflow atomicity, block structure/version, state-root enforcement, atomic block rollback, storage commit validation, malformed P2P frames, unknown outbound and inbound message types, oversized outbound payloads, and preservation of the prior output frame on decode failure, and inbound rejection tests for unknown types, unsupported versions, oversized declared lengths, short headers, and payload-length mismatches.
- Test assertions remain enabled in Release builds.

## Critical limitations

This is **not a functioning blockchain or testnet**. The OpenSSL-backed ML-DSA verifier is implemented and exercised by a real ML-DSA-65 test when OpenSSL 3.5+ is available. The generic ledger unit tests also retain a fake suite-1 verifier strictly for state-transition testing; it is not cryptography and must never be accepted by a production node. The real ML-DSA test uses reserved test-only suite ID 0x7FFE. No production ML-DSA suite ID, activation rule, or wallet key-management workflow has been approved.

No persistent LedgerStore backend is implemented: the new interface is a contract only, not durable storage. ADR-0011 documents the required state-replay trust boundary; matching a caller-supplied state root alone is insufficient. Consensus/fork choice/finality, mining or validator selection, live P2P sockets/peer manager, wallet, RPC, genesis rules, and security audit remain unimplemented. The current state-root and block-header V2 formats are prototypes and are not frozen production consensus rules. Structural block validation does not yet execute transactions or verify block authorization/consensus. Do not use this prototype for funds or production.

See:
- `docs/ADR-0002-PQC-ACCOUNT-IDENTITY.md`
- `docs/ADR-0003-DURABLE-LEDGER-STORAGE.md`
- `docs/ADR-0007-DETERMINISTIC-STATE-ROOT.md`
- `docs/ADR-0008-LEDGER-STORE-INTERFACE.md`
- `docs/ADR-0004-TRANSACTION-V3-WIRE-FORMAT.md`
- `docs/ADR-0005-OPENSSL-ML-DSA-BACKEND.md`
- `docs/ADR-0006-ATOMIC-BLOCK-EXECUTION.md`
- `docs/PROTOCOL_DECISIONS.md`

## Build

Requirements: CMake 3.20+, a C++20 compiler, and OpenSSL 3.x development headers/libraries. Provider-backed ML-DSA support and its real-crypto test are enabled automatically when OpenSSL 3.5+ is found; older OpenSSL versions build the non-ML-DSA foundation only.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
```

Validation status: an earlier source snapshot passed a local CMake Release build and CTest using GCC 14.2.0 and OpenSSL 3.5.7. That result predates the latest P2P changes and must not be treated as validation of the current branch. The latest branch revision has not yet been compiled or tested. The repository could not be cloned in the earlier environment because GitHub DNS resolution failed. The manually triggered workflow is `.github/workflows/native-ci.yml`; it has not been dispatched in order to conserve the repository's limited Actions minutes.

## Design constraints

- No Ethereum/EVM dependency in native consensus or execution.
- No floating-point values in consensus-critical amounts.
- Canonical, versioned serialization only.
- Standardized, reviewed cryptography only; no home-grown signatures.
- Node identity must not depend on MAC address, IMEI, or hardware serial numbers.
- Monetary and consensus rules must be specified before mainnet.
