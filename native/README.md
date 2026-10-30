# HJZ Native Layer-1 — Foundation

This directory contains the early C++20 implementation of an independent Layer-1. It is deliberately separate from the legacy ERC-20 prototype in `contracts/`; the ERC-20 contract is not the native HJZ coin.

## Current prototype scope

- Canonical block-header and Transaction V3 serialization.
- Transaction V3 carries an explicit authorization-suite ID and bounded public-key bytes.
- Account addresses are derived from suite ID + canonical public key using domain-separated SHA3-256; ledger rejects sender/key mismatches.
- Chain-ID binding, integer amount/fee fields, sequential account nonces, and deterministic signing payload.
- SHA3-256 through OpenSSL EVP.
- Domain-separated transaction IDs, block hashes, and binary Merkle-tree hashing.
- Deterministic ledger transitions with balance, fee, nonce, replay, overflow, and atomic candidate-state checks.
- Block transaction executor applies transactions in canonical block order and commits the resulting ledger state only if every transaction succeeds; a rejected transaction leaves the original block state untouched.
- Explicit verifier capability check; unknown authorization suites are rejected.
- OpenSSL 3.5+ ML-DSA verifier for ML-DSA-44/65/87 with strict public-key and signature size checks.
- Conditional real ML-DSA-65 key-generation, signing, verification, and ledger-authorization test when OpenSSL 3.5+ is available.
- Structural block checks for height, parent linkage, timestamp bounds, transaction count, duplicates, and Merkle root.
- Bounded P2P frame codec with protocol magic/version, typed messages, length checks, and a 2 MiB payload cap.
- Unit tests for hashing, address derivation, key/address mismatch, suite rejection, ledger success/replay/chain/signature failures, recipient-overflow atomicity, block structure, and malformed P2P frames.
- Test assertions remain enabled in Release builds.

## Critical limitations

This is **not a functioning blockchain or testnet**. The OpenSSL-backed ML-DSA verifier is implemented and exercised by a real ML-DSA-65 test when OpenSSL 3.5+ is available. The generic ledger unit tests also retain a fake suite-1 verifier strictly for state-transition testing; it is not cryptography and must never be accepted by a production node. The real ML-DSA test uses reserved test-only suite ID 0x7FFE. No production ML-DSA suite ID, activation rule, or wallet key-management workflow has been approved.

Persistent storage, consensus/fork choice/finality, mining or validator selection, live P2P sockets/peer manager, wallet, RPC, state roots, genesis rules, and security audit are not implemented. Structural block validation does not yet execute transactions or verify block authorization/consensus. Do not use this prototype for funds or production.

See:
- `docs/ADR-0002-PQC-ACCOUNT-IDENTITY.md`
- `docs/ADR-0003-DURABLE-LEDGER-STORAGE.md`
- `docs/ADR-0004-TRANSACTION-V3-WIRE-FORMAT.md`
- `docs/PROTOCOL_DECISIONS.md`

## Build

Requirements: CMake 3.20+, a C++20 compiler, and OpenSSL 3.x development headers/libraries. Provider-backed ML-DSA support and its real-crypto test are enabled automatically when OpenSSL 3.5+ is found; older OpenSSL versions build the non-ML-DSA foundation only.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
```

The current source snapshot was compiled in a local CMake Release harness using GCC 14.2.0 and OpenSSL 3.5.7; CTest passed 1/1. The repository could not be cloned in that environment because GitHub DNS resolution failed, so the local harness was assembled from the branch's fetched source files rather than a full checkout. This was not a GitHub Actions run; the manually triggered workflow is `.github/workflows/native-ci.yml` and its run remains pending.

## Design constraints

- No Ethereum/EVM dependency in native consensus or execution.
- No floating-point values in consensus-critical amounts.
- Canonical, versioned serialization only.
- Standardized, reviewed cryptography only; no home-grown signatures.
- Node identity must not depend on MAC address, IMEI, or hardware serial numbers.
- Monetary and consensus rules must be specified before mainnet.
