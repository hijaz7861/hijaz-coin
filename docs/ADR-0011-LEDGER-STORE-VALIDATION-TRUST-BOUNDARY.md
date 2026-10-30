# ADR-0011: Ledger Store Validation Trust Boundary

Status: required design constraint; backend implementation pending.
Date: 2026-10-30

## Problem

The current validate_atomic_block_commit function checks:
- supported block header version;
- equality between the block parent hash and caller-supplied expected parent hash;
- transaction structural validity and uniqueness;
- transaction Merkle root;
- equality between the supplied resulting state's state root and the block header state root.

It does NOT prove that the supplied resulting state was produced by executing the block's transactions against the store's actual pre-commit state. A caller could provide an unrelated state and a matching state root. Hash equality is a commitment check, not a state-transition proof.

## Decision

A durable LedgerStore implementation MUST NOT treat validate_atomic_block_commit alone as authorization to advance the canonical chain.

Before committing, the backend or a tightly controlled validation service must:
1. Begin a serialized write transaction and read the current canonical tip and canonical ledger state from the same database snapshot.
2. Compare the stored tip with the expected parent and require the next contiguous height. For an uninitialized database, only the explicitly defined genesis/bootstrap path may initialize the chain.
3. Re-execute every block transaction in order against a candidate copy of the stored state using the configured chain ID and a production SignatureVerifier.
4. Reject any transaction failure; no partial state may be committed.
5. Compute the resulting deterministic state root and compare it with the block header state root.
6. Compare the re-executed candidate state with the caller-supplied resulting state, or remove the caller-supplied state from the trust boundary and persist only the internally computed state.
7. Atomically write block bytes, canonical height index, transaction indexes, state rows, accumulated fees, state root, chain tip, and undo data.
8. Commit only after all checks and writes succeed. Any error must roll back every table mutation.

The preferred API is to have the store compute the resulting state internally from its persisted parent state. If an API continues accepting resulting_state, that value is an assertion to compare against the internally computed result, never an authority.

## Signature verifier and chain configuration

The store must receive or own immutable chain configuration and a production verifier. Test-only fake verifiers must never be configured in a production node. The chain ID, enabled authorization suites, and signature backend configuration must be explicit and fixed for the store's lifetime or changed only through a specified protocol upgrade.

## Concurrency

The expected-parent comparison must occur inside the same serialized database transaction that writes the new tip. A preflight read followed by a later write without rechecking permits two writers to race and publish conflicting children.

## Genesis

Genesis initialization is a separate, one-time operation. It must verify a configured genesis block and genesis state commitment, enforce an empty database, and commit all genesis records atomically. An all-zero parent hash alone is not sufficient authorization to initialize arbitrary state.

## Required tests

- A supplied state with a matching state root but not produced by block execution is rejected.
- A valid block is replayed from the persisted parent state and committed.
- A transaction with a bad signature leaves every table and the tip unchanged.
- A block with a valid Merkle root but incorrect state transition is rejected.
- Two writers using the same parent cannot both commit.
- Restart and replay produce the same state root.
- Genesis can be initialized once only; reinitialization is rejected.
- Failure injection at each write stage rolls back all canonical data.

## Consequence

The current abstract interface and pre-commit validator are scaffolding only. They are insufficient to establish a secure durable commit boundary until execution is tied to the database's persisted parent state. No durable-storage or production-readiness claim is permitted before these requirements are implemented and tested.
