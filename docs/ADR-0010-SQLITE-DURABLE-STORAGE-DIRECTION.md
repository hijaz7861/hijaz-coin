# ADR-0010: Initial Durable Storage Backend Direction

Status: SQLite selected as the first implementation candidate; backend NOT implemented.
Date: 2026-10-30

## Context

The native node needs one crash-consistent commit boundary for canonical block bytes, canonical height mapping, resulting account state, transaction indexes, chain tip, state root, and future undo data. The existing LedgerStore is only an abstract interface and does not persist anything.

The first backend should be practical on Linux x86-64 and Linux ARM64, testable in constrained development environments, and suitable for a single-node writer with concurrent readers. Android support is a build target to validate separately, not an assumption that the platform's private SQLite library is a stable NDK dependency.

## Options

### SQLite
- Embedded C library with a transactional SQL interface and mature crash-recovery behavior.
- WAL mode allows readers to continue against a consistent snapshot while a writer appends changes; SQLite still serializes writers.
- A single SQL transaction can atomically publish the block, indexes, state rows, undo data, and canonical tip.
- Small integration surface and low operational burden for a first node implementation.
- Limits: one writer at a time, database size and write amplification need measurement, WAL must remain on a local filesystem, and large full-state rewrites would be inefficient.

### RocksDB
- Embedded LSM key-value store with strong suitability for high-volume key/value workloads and configurable write batches.
- Official platform requirements list Linux ARM64 support.
- More operational tuning, compaction behavior, dependency/build complexity, and recovery semantics to validate for the required multi-key atomic commit.
- Keep as a future alternative if measured workload demonstrates SQLite is insufficient.

## Decision

Use SQLite as the initial backend implementation candidate, behind the existing LedgerStore interface. This is a reversible engineering choice, not a frozen consensus rule and not a claim that SQLite has already been integrated.

The initial implementation should:
- use a pinned, supported SQLite release and the C API;
- use a local database file, never a network filesystem for WAL;
- enable WAL and explicitly choose/document synchronous durability settings;
- use BEGIN IMMEDIATE (or an equivalent writer reservation) for canonical commits;
- enforce a schema version and transactional migrations;
- store fixed-width hashes and canonical serialized blocks as BLOBs;
- use prepared statements and bound parameters only;
- keep all canonical indexes, ledger state, transaction indexes, tip metadata, and undo records in the same database transaction;
- compare the expected parent hash with the stored canonical tip inside the transaction;
- validate the block and resulting state before opening the write transaction, then re-check concurrency-sensitive invariants inside it;
- reject duplicate block heights and transaction IDs using database constraints;
- expose typed errors without leaking SQL text or secret material.

## Initial logical schema

The first migration should define, at minimum:
- metadata(key, value) for schema version and canonical tip metadata;
- blocks(block_hash PRIMARY KEY, height UNIQUE, parent_hash, state_root, canonical_bytes);
- canonical_chain(height PRIMARY KEY, block_hash UNIQUE);
- accounts(address PRIMARY KEY, balance, next_nonce);
- chain_state(singleton PRIMARY KEY, accumulated_fees, state_root);
- transactions(txid PRIMARY KEY, block_hash, tx_index);
- undo_data(block_hash PRIMARY KEY, encoded_undo).

All integer amounts, balances, fees, heights, and nonces must use a representation that preserves the full unsigned 64-bit domain. SQLite INTEGER is signed 64-bit; therefore unsigned protocol values above INT64_MAX must be encoded in fixed-width big-endian BLOBs or rejected by an explicit protocol bound. The implementation must not silently cast uint64_t to SQLite INTEGER.

## Durability profile

The initial node profile should use WAL with synchronous=FULL unless platform-specific testing justifies a documented alternative. NORMAL can improve throughput but may lose the most recent committed transactions after power loss even though database consistency is preserved. Consensus state must never be acknowledged as durably committed under a weaker profile without making that trade-off explicit.

WAL files must be kept beside the database on a local filesystem. Backup/export procedures must use SQLite's online backup API or a checkpointed, closed database; copying only the main database file while WAL is active is not a valid backup procedure.

## Required tests before calling storage durable

1. Restart/reopen and deterministic state replay.
2. Process termination at injected points during a multi-table commit.
3. Simulated write failures and rollback verification.
4. Stale expected-parent conflict under concurrent writers.
5. Duplicate block height and transaction ID constraint tests.
6. Corrupt block/state row detection and startup integrity checks.
7. Schema migration and downgrade refusal tests.
8. WAL checkpoint, backup, and restore tests.
9. Large-state and append/read benchmarks on Linux x86-64 and ARM64.
10. Android NDK build/integration validation if Android remains a supported node target.

## References

- SQLite transactional guarantees: https://www.sqlite.org/transactional.html
- SQLite WAL behavior and limitations: https://www.sqlite.org/wal.html
- SQLite isolation and serialized writers: https://www.sqlite.org/isolation.html
- Android SQLite WAL guidance: https://developer.android.com/reference/android/database/sqlite/SQLiteDatabase.html
- RocksDB platform requirements: https://github.com/facebook/rocksdb/wiki/Platform-Requirements

## Consequences

SQLite is the first concrete backend to implement and benchmark. RocksDB remains a possible later backend behind the same interface. Neither the abstract interface nor this ADR constitutes an implemented durable backend. No production node or testnet may be declared ready until the implementation passes the required crash/recovery and adversarial tests.
