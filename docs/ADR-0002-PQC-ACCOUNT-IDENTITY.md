# ADR-0002: Post-Quantum Account Identity and Transaction Authorization

Status: proposed protocol decision; implementation required before production signatures.
Date: 2026-10-30

## Context

The current transaction prototype stores `sender` as a 32-byte `Hash256` and passes that value to the injected `SignatureVerifier` as though it were a public key. This is only a test abstraction. It cannot represent standardized ML-DSA public keys, and it does not define how a public key is bound to an account address.

A production transaction format must make authorization deterministic, unambiguous, versioned, and independently verifiable by every node.

## Proposed identity model

1. An account address is a 32-byte identifier, not a public key.
2. A transaction carries an explicit authorization-suite identifier and the canonical public-key bytes needed to verify that transaction.
3. The sender address is derived from the authorization suite and canonical public key using a domain-separated hash, conceptually:

   `address = H("HJZ/ADDRESS/V1" || suite_id || canonical_public_key)`

4. A validator rejects a transaction if the derived address does not equal its declared sender address.
5. Public-key encoding, suite identifiers, address derivation, and signature encoding are consensus-critical protocol rules. They must have exact byte-level specifications and test vectors before activation.
6. A transaction signing payload includes the chain ID, transaction format version, authorization-suite identifier, sender and receiver addresses, amount, fee, nonce, and any other consensus-critical transaction fields. It excludes the signature itself.
7. The transaction identifier commits to the complete canonical transaction, including authorization metadata, public key, and signature. Domain separation is mandatory.
8. Unknown or disabled suite identifiers are rejected; nodes must not silently downgrade to another algorithm.

## Post-quantum algorithm policy

- Evaluate NIST FIPS 204 ML-DSA for transaction signatures.
- Evaluate NIST FIPS 203 ML-KEM only if/when the peer transport requires post-quantum key establishment. ML-KEM is not a transaction-signature algorithm.
- Keep suite identifiers and serialization versioned so algorithm migration can be specified without ambiguous interpretation.
- Use a maintained, independently reviewed cryptographic implementation. Do not implement lattice cryptography in project code.
- Select the exact ML-DSA parameter set only after benchmarking, implementation availability review, threat modelling, and a documented protocol decision.
- Do not describe any scheme as unbreakable or permanently quantum-proof.

## Key lifecycle

The protocol specification must define account recovery, key rotation, multi-key authorization, and the effect of compromised keys before mainnet. Rotation must itself be authorized by the currently valid account policy or by an explicitly specified recovery mechanism. There is no implicit administrator override.

## Transaction validation order

A production validator should, in a deterministic order:

1. Check transaction format and bounded field lengths.
2. Check chain ID and supported authorization-suite identifier.
3. Parse the canonical public key for that suite.
4. Derive the sender address and compare it to the declared sender.
5. Reconstruct the exact canonical signing payload.
6. Verify the signature using the selected standardized implementation.
7. Apply nonce, balance, fee, replay, and state-transition rules atomically.

Every failure must leave candidate state unchanged.

## Compatibility and migration

The current `Hash256 sender + signature bytes` structure is a prototype and must not be treated as a production wire format. Before introducing production PQC verification, introduce a new transaction format version containing the suite identifier and public-key bytes. Do not reinterpret old serialized bytes in place. Legacy prototype transactions must be rejected by a production network unless an explicit, separately reviewed migration rule is adopted.

## Required tests

- Known-answer address derivation vectors for each enabled suite.
- Valid and invalid signature vectors from the chosen cryptographic library.
- Wrong-key / sender-address mismatch rejection.
- Chain-ID and transaction-domain separation tests.
- Truncated, oversized, malformed, and non-canonical public-key rejection.
- Unknown-suite and downgrade rejection.
- Signature mutation and signing-payload mutation rejection.
- Atomic-state tests proving all authorization failures leave balances and nonces unchanged.

## Decision gate

This ADR is a design constraint, not a claim that PQC is implemented. No mainnet or incentivized public testnet should launch until the exact suite, canonical encodings, key lifecycle, cryptographic library, audit plan, and protocol test vectors are approved and implemented.
