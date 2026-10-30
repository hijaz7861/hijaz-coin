# ADR-0013: Durable Block Context Validation

Status: required design constraint; durable backend implementation pending.
Date: 2026-10-30

## Context

The current storage preflight validates header version 2, equality between the header parent hash and caller-supplied expected parent hash, transaction structural validity and uniqueness, transaction Merkle root, and equality between the caller-supplied state root and the header state root.

It does not validate the block height or timestamp against the actual persisted parent record. The standalone structural validator does check expected height, parent timestamp, and an adjusted-network-time upper bound, but a durable store must not rely on a caller having invoked that helper correctly before commit.

A timestamp check also needs an explicit time policy. The backend cannot safely invent an adjusted network time or silently substitute an arbitrary local wall clock as a consensus rule.

## Decision

A durable LedgerStore commit path MUST validate block context using values read from the same serialized database transaction that performs the commit:

1. Read the canonical tip height, hash, state root, and parent timestamp under the write transaction.
2. Require the candidate height to be exactly parent height + 1, with checked overflow. Genesis must use its separate one-time bootstrap path.
3. Require the candidate previous hash to equal the actual canonical parent hash.
4. Validate timestamp strictly greater than the parent timestamp.
5. Apply the protocol-defined future-drift rule using an explicit adjusted-network-time input and immutable BlockValidationLimits; the protocol must specify how adjusted network time is derived and protected from manipulation.
6. Enforce transaction-count and all other block structural limits.
7. Recompute transaction Merkle root, execute transactions from the database's own parent state with the configured production verifier, and verify the resulting state root.
8. Persist the computed state and all canonical indexes atomically, rechecking no caller-supplied context outside the transaction is treated as authority.

The commit API must make its time-validation context explicit (for example, an adjusted-network-time argument plus immutable validation limits), or delegate that context to a trusted consensus-validation component whose result is bound to the exact block and parent snapshot. It must not accept a bare boolean such as is_valid from an untrusted caller.

## Required tests

- Wrong candidate height is rejected without changing any stored row.
- Height increment overflow is rejected.
- Timestamp equal to or earlier than parent timestamp is rejected.
- Timestamp beyond the permitted future drift is rejected.
- Adjusted-network-time boundary values are tested, including uint64 overflow.
- Two concurrent writers cannot validate against different parent snapshots and both commit.
- A block passing standalone preflight but failing persisted-context validation is rejected atomically.
- Restart preserves parent timestamp and subsequent validation behavior.

## Consequences

The abstract storage interface is scaffolding and does not yet enforce these context rules. A SQLite implementation must not be considered secure or complete until these checks are implemented within the serialized commit transaction and tested. The protocol's adjusted-network-time derivation remains an open consensus specification item.
