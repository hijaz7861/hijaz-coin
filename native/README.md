# HJZ Native Layer-1 — Foundation

This directory begins a new, independent C++20 Layer-1 implementation. It is deliberately separate from the legacy ERC-20 prototype in `contracts/`.

## Current scope

- Canonical, versioned transaction and block-header serialization.
- Chain-ID binding, integer amount/fee fields, and sequential account nonces.
- SHA3-256 through OpenSSL EVP (not a custom cryptographic primitive).
- Domain-separated transaction IDs, block hashes, and binary Merkle-tree hashing.
- Deterministic account-ledger state transition with balance, fee, nonce, overflow, and replay checks.
- Atomic state application: a rejected transaction does not mutate the supplied state.
- An injected signature-verification interface. Unit tests use a deliberately fake verifier only to test ledger behavior.

## Explicit limitations

This is **not a functioning blockchain or testnet**. The ledger's signature interface has no production implementation. ML-DSA/ML-KEM, persistent storage, consensus, proof-of-work or validator selection, fork choice, P2P networking, wallet, RPC, and peer privacy are not implemented. The unit-test verifier is not cryptography. Do not use this code for funds or production.

## Build

Requirements: CMake 3.20+, a C++20 compiler, and OpenSSL 3.x development headers/libraries.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
```

## Design constraints

- The native chain must not depend on Ethereum/EVM for consensus or execution.
- No floating-point values in consensus-critical amounts.
- Serialization must be canonical and versioned.
- Cryptographic algorithms must be standardized, reviewed, and replaceable through explicit protocol versioning; no home-grown signatures.
- Node identity must be cryptographic and independent of MAC address, IMEI, or hardware serial numbers.
- Monetary and consensus rules must be specified before mainnet implementation.
