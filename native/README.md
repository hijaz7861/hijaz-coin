# HJZ Native Layer-1 — Foundation

This directory begins a new, independent C++20 Layer-1 implementation. It is deliberately separate from the legacy ERC-20 prototype in `contracts/`.

## Current scope

- Canonical, fixed-width transaction and block-header serialization.
- SHA3-256 through OpenSSL EVP (not a custom cryptographic primitive).
- Domain-separated transaction IDs, block hashes, and binary Merkle-tree hashing.
- Integer-only amount representation.
- A small executable test target.

## Explicitly not implemented yet

This foundation is **not a functioning blockchain or testnet**. It does not yet implement signature verification, ML-DSA/ML-KEM, UTXO/account state, fees, replay protection, consensus, proof-of-work, fork choice, P2P networking, persistent storage, wallet, RPC, or peer privacy. Structural transaction checks are not authorization checks. Do not use it for funds or production.

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
- Consensus and monetary rules must be specified before mainnet implementation.
