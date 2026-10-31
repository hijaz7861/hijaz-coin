# Hijaz Coin — Native Layer-1 Engineering Roadmap

## Product direction

Hijaz Coin (HJZ) is intended to become an independent native Layer-1 network. The existing ERC-20 contract is retained as a legacy prototype and is not the native coin implementation.

## Architecture workstreams

1. Protocol specification: monetary units, issuance, transaction lifecycle, fees, block limits, timestamp rules, upgrade/version policy, and threat model.
2. Cryptography: standardized post-quantum signatures (evaluate FIPS 204 ML-DSA parameter sets), key encapsulation (FIPS 203 ML-KEM where transport key establishment is needed), SHA3/SHAKE hashing, secure key handling, algorithm agility, and independent review.
3. Ledger: deterministic state transition, balance/UTXO model decision, nonce/replay rules, conservation invariants, fee accounting, and atomic persistence.
4. Consensus: evaluate Bitcoin-like adaptive Proof-of-Work as the preferred candidate direction. ADR-0014 records the proposed difficulty-retarget, target-validation, timestamp, cumulative-chain-work, fork-choice, economic-analysis, and adversarial-test requirements. PoW is not yet adopted; all consensus parameters remain open until a final consensus ADR is approved.
5. Networking: authenticated P2P handshake, peer discovery, bounded messages, DoS controls, encrypted transport, gossip validation, and optional Tor/I2P support.
6. Node operations: full-node mode, optional block production/mining, safe configuration, structured minimal logs, and hardware-independent cryptographic node identity.
7. Wallet and APIs: key generation/backup, signing, address encoding, transaction construction, local RPC, and permission boundaries.
8. Verification: deterministic unit/property/fuzz tests, multi-node partition/reorg tests, independent cryptographic audit, reproducible builds, and testnet monitoring.

## Current implementation checkpoint

The `native/` directory contains a C++20 prototype foundation: SHA3-256 via OpenSSL EVP, Transaction V3, provider-backed ML-DSA verification when OpenSSL 3.5+ is available, block header V2 with a deterministic state-root commitment, ordered all-or-nothing block execution, structural block validation, a bounded P2P frame codec, and unit tests. A vendor-neutral storage interface is present, but no durable storage backend, consensus implementation, live P2P node, or testnet exists.

ADR-0010 selects SQLite as the first durable-storage implementation candidate, subject to actual implementation, crash/recovery testing, and Linux ARM64 validation. ADR-0011 requires the store to replay transactions from its own persisted parent state with a production signature verifier; merely comparing a caller-supplied state root is not sufficient. These are architecture decisions, not completed storage.

## Release gates

- No mainnet or real-value use before protocol review, threat modeling, independent cryptographic review, fuzzing, multi-node testnet, and documented recovery/upgrade procedures.
- Never describe SHA3-256 as a post-quantum signature. Hash functions and public-key signature schemes have different quantum threat models.
- Do not identify nodes by MAC address, IMEI, or device serial numbers.
