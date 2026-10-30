# ADR-0002: Post-Quantum Account Identity and Transaction Authorization

Status: partially implemented prototype; production cryptography and suite selection remain open.
Date: 2026-10-30

## Context

The initial transaction prototype treated a 32-byte sender hash as if it were a public key. That cannot represent ML-DSA public keys and did not bind an account identifier to the key authorizing a spend.

## Implemented prototype model

The current Transaction V3 structure carries:
- a one-byte format version (currently 3),
- a two-byte authorization-suite identifier,
- bounded canonical public-key bytes,
- a 32-byte sender account address,
- chain ID, receiver, amount, fee, nonce, and signature bytes.

The sender address is derived from the suite identifier and public key using the domain-separated SHA3-256 helper:

`address = domain_hash("HJZ/ADDRESS/V1", u16_be(suite_id) || canonical_public_key)`

The ledger recomputes this address and rejects a mismatch before authorization or state mutation. Public-key bytes and signatures are each bounded to 16 KiB in this prototype. The exact byte-level format is specified in ADR-0004.

The `SignatureVerifier` interface now receives the suite identifier and full public-key bytes. It must explicitly report whether it supports a suite; unsupported suites are rejected. An OpenSSL 3.5+ provider-backed verifier now supports the standardized ML-DSA-44, ML-DSA-65, and ML-DSA-87 parameter sets. Tests generate a real ML-DSA-65 keypair, sign a transaction payload, import the raw public key, and verify the transaction through the ledger. The real-crypto test uses suite ID `0x7FFE`, which is test-only and is not a registered production suite. The older fake suite-1 verifier remains only for generic ledger state-transition tests.

## Post-quantum algorithm policy

- Evaluate NIST FIPS 204 ML-DSA for transaction signatures.
- Evaluate NIST FIPS 203 ML-KEM only if/when peer transport requires post-quantum key establishment. ML-KEM is not a transaction-signature algorithm.
- Do not implement lattice cryptography in project code.
- Select an exact ML-DSA parameter set and assign a production suite ID only after reviewing maintained library support, canonical encodings, benchmarks, interoperability vectors, and threat model.
- Keep algorithm identifiers explicit and reject unknown/disabled suites without downgrade.
- Do not describe any scheme as unbreakable or permanently quantum-proof.

## Key lifecycle

Before mainnet, specify key rotation, recovery, multi-key authorization, compromised-key handling, and whether account policy can change authorization suites. There is no implicit administrator override.

## Validation order

1. Check format version and bounded field lengths.
2. Check chain ID.
3. Recompute sender address from suite ID and canonical public key; reject mismatch.
4. Check whether the configured verifier supports the suite.
5. Reconstruct the exact canonical signing payload.
6. Verify signature using the standardized implementation.
7. Apply nonce, balance, fee, replay, and state-transition rules atomically.

All failures must leave ledger state unchanged.

## Compatibility

Transaction V3 is a new prototype format. Do not reinterpret V2 serialized bytes as V3. Legacy prototype transactions must be rejected by a production network unless an explicit migration rule is separately reviewed and adopted.

## Required production tests

- Published known-answer address derivation vectors for each enabled suite.
- Standard valid/invalid signature vectors from the chosen cryptographic implementation.
- Wrong-key / sender-address mismatch rejection.
- Chain-ID and transaction-domain separation tests.
- Truncated, oversized, malformed, and non-canonical public-key rejection.
- Unknown-suite and downgrade rejection.
- Signature and signing-payload mutation rejection.
- Atomic-state tests proving authorization failures do not mutate balances or nonces.

## Decision gate

This ADR does not claim production PQC is implemented. No public testnet with meaningful incentives or mainnet should launch until the exact suite, canonical encodings, key lifecycle, library, interoperability vectors, audit plan, and migration policy are implemented and reviewed.
