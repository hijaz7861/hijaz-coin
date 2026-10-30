# ADR-0008: Vendor-Neutral Atomic Ledger Store Interface

Status: interface implemented; durable backend not implemented.
Date: 2026-10-30

## Decision

Define a database-vendor-neutral `LedgerStore` boundary. Consensus and deterministic execution must not depend on RocksDB, SQLite, or another storage engine.

The interface exposes:
- canonical tip metadata,
- canonical height-to-block-hash lookup,
- block lookup by hash,
- current ledger-state reads,
- atomic publication of a validated block and its resulting state.

## Atomic commit contract

`AtomicBlockCommit` contains the expected parent hash, complete block, and resulting ledger state. A conforming implementation must publish all of the following as one atomic transaction or equivalent crash-safe operation:

1. Canonical serialized block bytes and block hash.
2. Canonical height-to-hash mapping.
3. Complete resulting account state and accumulated fees.
4. Transaction inclusion indexes.
5. Header state root and canonical tip metadata.
6. Undo data required by the eventual reorganization design.

If any write fails, none of these changes may become visible. The implementation must compare `expected_parent_hash` against the currently stored canonical tip to prevent stale writers from silently advancing the chain.

The interface distinguishes a tip conflict, missing parent, corrupt data, and generic storage failure.

## Trust boundary

Only blocks that have passed:
- structural validation,
- transaction signature verification,
- deterministic transaction execution,
- state-root comparison,
- and the selected consensus/finality checks

may be passed to the storage commit boundary. The storage layer itself must not decide validity or fork choice.

## What is implemented

The C++ header `native/include/hijaz/storage.hpp` defines the abstract interface and atomic commit contract. It intentionally introduces no database dependency.

## What is not implemented

There is no RocksDB, SQLite, filesystem journal, in-memory production backend, crash recovery, schema migration, snapshot engine, reorganization/undo implementation, or transaction index implementation. The abstract interface alone does not provide durability and must not be described as persistent storage.

## Next implementation gate

Before choosing a backend:
- verify exact supported Linux and ARM64/Android targets,
- benchmark account lookup, block append, range scans, and state snapshots,
- test crash consistency and atomic batches,
- verify licensing and security maintenance,
- establish schema versioning and migration policy.

A concrete backend must pass process-kill/restart recovery tests, injected write-failure tests, deterministic replay tests, reorganization tests, corruption tests, and concurrent-reader tests before a node can be described as durable.
