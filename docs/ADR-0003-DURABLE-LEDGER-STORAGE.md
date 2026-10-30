# ADR-0003: Durable Ledger Storage and Crash Recovery

Status: proposed architecture; no database implementation is present.
Date: 2026-10-30

## Goals

The native node needs deterministic, crash-safe persistence for accepted blocks, canonical chain metadata, account state, and transaction indexes. A restart must never expose a partially applied block or silently select a different canonical history.

## Storage boundary

Keep consensus logic independent of the database vendor. Introduce a storage interface with operations equivalent to:

- Read the current canonical tip and finalized checkpoint.
- Read a block by hash and canonical block hash by height.
- Read account state by address.
- Read transaction inclusion/status by transaction ID.
- Commit a validated block and its complete state transition atomically.
- Reorganize the canonical chain using an explicit undo/replay plan.
- Create and verify a versioned snapshot.
- Run consistency checks and report corruption; never silently repair consensus data by guessing.

The storage layer must not decide whether a block is valid or canonical. Those decisions belong to deterministic consensus and validation modules.

## Atomic block commit

A block commit must be one database transaction or equivalent atomic write batch containing:

1. The serialized block and block hash.
2. The canonical height-to-hash mapping update.
3. The resulting account-state changes.
4. Transaction inclusion indexes.
5. The new state root and canonical tip metadata.
6. Any undo data required by the selected reorganization model.

The commit must either become fully visible or have no effect. A process crash during commit must recover to the previous complete state or the new complete state, never a mixture.

## Reorganization and undo

Before a consensus/fork-choice algorithm is selected, define the storage API so that a reorganization can:
- identify the common ancestor,
- disconnect old canonical blocks in reverse order using persisted undo information,
- connect the new branch in forward order,
- atomically publish the resulting canonical tip.

Do not implement an ad hoc “longest chain wins” policy. Fork choice and finality are consensus decisions, not storage heuristics.

## Candidate engine evaluation

Evaluate a mature embedded transactional key-value engine (RocksDB is a candidate) against SQLite for the expected access pattern. Compare:
- atomic batch and crash-recovery guarantees,
- write amplification and compaction behavior,
- range scans and prefix iteration,
- snapshot and backup support,
- mobile/ARM64 build availability,
- dependency size and operational complexity,
- licensing and security maintenance.

Do not add a database dependency until the target build environments and exact library versions are verified. Database choice must not alter consensus results.

## Key encoding and schema

- Use explicit schema/version prefixes for every key family.
- Use fixed-width, big-endian integers for ordered numeric keys.
- Use raw fixed-width hashes/addresses only where their lengths are specified by the protocol.
- Store canonical serialized blocks and transactions, not compiler-dependent object layouts.
- Reject unknown mandatory schema versions and malformed records.
- Bound record sizes before allocation.
- Store a schema version and protocol version in database metadata.

Suggested logical key families (names are illustrative, not wire-format commitments):
- `meta/schema_version`
- `meta/protocol_version`
- `meta/canonical_tip`
- `meta/finalized_checkpoint`
- `block/<block_hash>`
- `canonical_height/<u64_be>`
- `account/<address>`
- `tx/<txid>`
- `undo/<block_hash>`
- `snapshot/<snapshot_id>`

## State roots and snapshots

A deterministic state-root algorithm must be specified before state roots are consensus-critical. Iteration order, key encoding, empty-state behavior, and tree construction must be canonical. Snapshots must include chain ID, height, block hash, protocol version, schema version, state root, and a cryptographic checksum. A node must verify a snapshot against a trusted finalized checkpoint or replay enough history to establish trust; a checksum alone does not prove that a snapshot is valid.

## Operational safety

- Never store wallet private keys in the chain database.
- Do not treat database encryption as a substitute for access control or key management.
- Backups must be consistent snapshots, not copies of a live database directory unless the engine explicitly guarantees that procedure.
- On corruption or failed integrity checks, stop canonical state mutation and surface a clear error.
- Keep migrations explicit, versioned, testable, and reversible where feasible.

## Required tests before implementation is accepted

- Atomic commit under injected failures at every write stage.
- Process termination/restart recovery tests.
- Deterministic replay from genesis to the same state root.
- Reorganization across multiple blocks and rollback after interrupted reorg.
- Snapshot export/import with root verification.
- Corrupt/truncated record rejection and bounded-allocation tests.
- Schema migration tests across supported versions.
- Concurrent read/write tests consistent with the chosen node execution model.

## Decision gate

This ADR defines requirements only. No persistent storage, RocksDB/SQLite integration, state-root consensus rule, snapshot trust model, or crash-recovery behavior is implemented by this document. These must be implemented and tested before describing a node as durable or production-ready.
