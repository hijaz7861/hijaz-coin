# ADR-0006: Deterministic Atomic Block Transaction Execution

Status: implemented execution primitive; consensus integration remains pending.
Date: 2026-10-30

## Decision

A block's ordinary transactions execute in their serialized order against a candidate ledger state. The node publishes the candidate state only if every transaction is accepted. A failure at any transaction index rejects the block execution result and leaves the caller's original ledger state unchanged.

## Implemented API

`execute_block_transactions(block, state, ledger, verifier)`:
- Copies the input `LedgerState` to a candidate.
- Applies each transaction in vector order using the chain-bound ledger.
- Stops at the first rejected transaction and reports its index and `ApplyResult`.
- Does not mutate the input state on any failure.
- Moves the fully executed candidate state into the caller's state only after all transactions pass.

Structural block validation remains a separate prerequisite and must be called before execution. This helper does not validate consensus, block producer authorization, coinbase/reward rules, block weight, state roots, or finality.

## Tests

The unit tests cover:
- Sequential nonce execution for multiple transactions in one block.
- Correct balances and next nonce after a successful block.
- A second-transaction signature failure after the first transaction has passed.
- Proof that the failed block does not publish the first transaction's otherwise valid state changes.

The tests use the test verifier for generic ledger behavior; the separate ML-DSA test exercises a real signature verifier.

## Rationale

Transaction-level atomicity is insufficient if a node mutates its canonical ledger after each transaction and a later transaction fails. Candidate-state execution gives the caller a clear all-or-nothing boundary suitable for later database transactions and consensus validation.

## Remaining requirements

Before this primitive can be used by a production node, integrate it with:
- Full block validation and production signature verification.
- A consensus-selected block authorization and reward/fee policy.
- Deterministic state-root calculation and header commitment.
- Atomic persistent storage commit and crash recovery.
- Fork-choice/reorganization logic and undo data.
- Resource limits and adversarial block tests.

This ADR does not claim that a complete block-validation pipeline, durable node, or blockchain is implemented.
