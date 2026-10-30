# ADR-0007: Deterministic Ledger State Root and Block Header V2

Status: prototype implemented; not frozen for production consensus.
Date: 2026-10-30

## Decision

Define a deterministic state commitment over the account ledger. The current prototype uses a domain-separated SHA3-256 Merkle tree.

Each account leaf preimage is the fixed-width concatenation:
- 32-byte account address,
- 8-byte unsigned big-endian balance,
- 8-byte unsigned big-endian next nonce.

The account leaf is hashed under `HJZ/STATE/ACCOUNT/V1`. Accounts are traversed in the lexicographic order of the fixed-width address key in `std::map`. Internal nodes hash the 64-byte left/right child concatenation under `HJZ/STATE/NODE/V1`; an unpaired final child is duplicated. Empty account trees use `HJZ/STATE/EMPTY/V1`.

The final state-root preimage commits to:
- 8-byte account count,
- 8-byte accumulated-fee total,
- 32-byte account-tree root.

The final digest uses `HJZ/STATE/ROOT/V1`. The explicit account count prevents ambiguity from duplicate-last tree padding. All integer encodings are unsigned big-endian.

## Block header V2

Block header version 2 adds a 32-byte `state_root` field between transaction Merkle root and nonce in canonical serialization. Block hashes use domain `HJZ/BLOCK/V2`. The structural validator rejects any header version other than its configured supported version (default 2).

Block execution computes the candidate ledger state in transaction order, compares its root to the header's state root, and publishes no state if the root mismatches.

## Tests

- Empty-state root is deterministic.
- Different account insertion orders produce the same root.
- Changing a balance changes the root.
- Changing accumulated fees changes the root.
- Block execution accepts a matching state root.
- A mismatched root rejects the block without mutating caller state.
- Unsupported block header version is rejected.

## Limitations and required review

This is a prototype state commitment, not an audited or frozen consensus format. Before activation, specify genesis and account creation semantics, absent-versus-zero account canonicalization, proof format and proof-size limits, state migration/versioning, chain ID binding, resource limits, and independent test vectors. Consider whether a sparse Merkle tree or another authenticated data structure is preferable for proof generation and light clients.

No persistent state backend, state proof API, consensus/fork choice, or production activation is implemented by this ADR.
